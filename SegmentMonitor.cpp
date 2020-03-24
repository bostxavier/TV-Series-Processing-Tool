#include <QPainter>
#include <QMouseEvent>

#include <QRegularExpression>

#include <QDebug>
#include <QAction>
#include <QFont>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <cmath>

#include "SegmentMonitor.h"
#include "Episode.h"
#include "TextProcessor.h"

SegmentMonitor::SegmentMonitor(QWidget *parent)
  : QWidget(parent),
    m_step(40),
    m_segmentHeight(4),
    m_duration(1),
    m_currPosition(0),
    m_idxColor(0),
    m_annotMode(false),
    m_prevPosition(-1)
{
  resize(75000, 140);

  // contextual menu containing speaker as manually annotated
  QFont boldFont = QGuiApplication::font();
  boldFont.setBold(true);

  setContextMenuPolicy(Qt::CustomContextMenu);
  
  m_speakers = new QMenu;
  m_speakers->setStyleSheet("menu-scrollable: 1;");
  // "menu-height: 200px");

  m_newSpeakerAct = new QAction(tr("Add speaker"), this);
  m_newSpeakerAct->setFont(boldFont);
  m_anonymizeAct = new QAction(tr("Anonymize"), this);
  m_anonymizeAct->setFont(boldFont);
  m_removeAct = new QAction(tr("Remove"), this);
  m_removeAct->setFont(boldFont);
  m_insertAct = new QAction(tr("Insert"), this);
  m_insertAct->setFont(boldFont);
  m_splitAct = new QAction(tr("Split"), this);
  m_splitAct->setFont(boldFont);
  
  m_colors << QColor(219, 213, 185) << QColor(224, 199, 30);
  m_colors << QColor(186, 22, 63) << QColor(255, 250, 129);
  m_colors << QColor(181, 225, 174) << QColor(154, 206, 223);
  m_colors << QColor(249, 149, 51) << QColor(252, 169, 133);
  m_colors << QColor(193, 179, 215) << QColor(116, 161, 97);
  m_colors << QColor(133, 202, 93) << QColor(72, 181, 163);
  m_colors << QColor(69, 114, 147) << QColor(249, 150, 182);
  m_colors << QColor(117, 137, 191) << QColor(84, 102, 84);

  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(showContextMenu(const QPoint&)));
  connect(m_newSpeakerAct, SIGNAL(triggered()), this, SLOT(addNewSpeaker()));
  clearSpeakers();
  reset();
}

void SegmentMonitor::clearSpeakers()
{
  m_speakers->clear();
  m_sep = m_speakers->addSeparator();
  m_speakers->addAction(m_newSpeakerAct);
  m_speakers->addAction(m_anonymizeAct);
  // m_speakers->addAction(m_removeAct);
  // m_speakers->addAction(m_insertAct);
  // m_speakers->addAction(m_splitAct);
  m_speakers->addSeparator();
  
  m_idxColor = 0;
}

void SegmentMonitor::reset()
{
  for (int i(0); i < m_segmentationList.size(); i++)
    m_segmentationList[i].clear();
  m_segmentationList.clear();
  m_shotSize.clear();
  m_speechRate.clear();
  m_musicRate.clear();
  m_segColors.clear();
  m_segColors.insert("", QColor(255, 255, 255));
  m_segColors.insert("C-1", QColor(255, 255, 255));
  m_segColors.insert("S", QColor(255, 255, 255));
  m_idxColor = 0;
}

///////////////
// modifiers //
///////////////

void SegmentMonitor::setAnnotMode(bool annotMode)
{
  m_annotMode = annotMode;
  if (annotMode)
    m_segmentHeight = 8;
  else
    m_segmentHeight = 4;
    // m_segmentHeight = 8;
}

///////////////
// accessors //
///////////////

qreal SegmentMonitor::getRatio() const
{
  return m_ratio;
}

///////////
// slots //
///////////

void SegmentMonitor::processRefSpeakers(const QStringList &speakers)
{
  clearSpeakers();
  reset();

  for (int i(0); i < speakers.size(); i++) {
    // QString spkLbl = reduceSpkLabel(speakers[i]);
    QString spkLbl = speakers[i];
    m_speakers->addAction(new QAction(spkLbl, this));
    m_segColors[spkLbl] = m_colors[m_idxColor++ % m_colors.size()];
  }
}

void SegmentMonitor::processShots(QList<Segment *> shots, Segment::Source source)
{
  m_source = source;

  // initialize list of camera labels
  QStringList camLabels;
  for (int i(0); i < shots.size(); i++)
    camLabels.push_back(shots[i]->getLabel(source));

  // set colors for recurring shots; retrieve shot sizes and music rate
  for (int i(0); i < shots.size(); i++) {

    QString camera = shots[i]->getLabel(source);
    
    if (camLabels.count(camera) > 1 && !m_segColors.contains(camera))
      m_segColors[camera] = m_colors[m_idxColor++ % m_colors.size()];

    else if (camLabels.count(camera) == 1)
      m_segColors[camera] = QColor(255, 255, 255);
    
    Shot *shot = dynamic_cast<Shot *>(shots[i]);
    m_shotSize.push_back(shot->computeHeightRatio());
    m_musicRate.append(shot->getMusicRates());
  }

  m_selectedLabels.push_back(QString());
  m_segmentationList.push_back(shots);
}

void SegmentMonitor::processSpeechSegments(QList<Segment *> speechSegments, Segment::Source source)
{
  QList<Segment *> mergedSpeechSegments;
  qint64 interThresh(3000);

  // compute speech rate
  computeSpeechRate(speechSegments, m_segmentationList[0].last()->getEnd());

  // reset color index
  m_idxColor = 0;
  
  for (int i(0); i < speechSegments.size(); i++) {

    QString spkLbl = speechSegments[i]->getLabel(source);
    
    // set speaker color if not already done
    if (!m_segColors.contains(spkLbl))
      m_segColors[spkLbl] = m_colors[m_idxColor++ % m_colors.size()];
    
    qint64 position = speechSegments[i]->getPosition();
    qint64 end = speechSegments[i]->getEnd();

    // possibly merge consecutive segments
    if (i > 0) {
      
      Segment *prevSegment = mergedSpeechSegments.last();
      QString prevSpkLbl = prevSegment->getLabel(source);
      qint64 prevEnd = prevSegment->getEnd();

      if (prevSpkLbl == spkLbl && position - prevEnd <= interThresh)
	prevSegment->setEnd(end);
   
      else {
	SpeechSegment *speechSegment = dynamic_cast<SpeechSegment *>(speechSegments[i]);
      mergedSpeechSegments.push_back(new SpeechSegment(position,
						       end,
						       QString(),
						       speechSegment->getSpeakers(),
						       speechSegment->getInterLocs()));
      }
    }

    else {
      SpeechSegment *speechSegment = dynamic_cast<SpeechSegment *>(speechSegments[i]);
      mergedSpeechSegments.push_back(new SpeechSegment(position,
						       end,
						       QString(),
						       speechSegment->getSpeakers(),
						       speechSegment->getInterLocs()));
    }
  }

  // reduce names to: initial first name + last name
  for (int i(0); i < mergedSpeechSegments.size(); i++) {
    
    QString spkLbl = mergedSpeechSegments[i]->getLabel(source);
    QString newSpkLbl = reduceSpkLabel(spkLbl);
    mergedSpeechSegments[i]->setLabel(newSpkLbl, source);
  }

  m_selectedLabels.push_back(QString());
  // m_segmentationList.push_back(mergedSpeechSegments);
  m_segmentationList.push_back(speechSegments);
  m_step = height() / (m_segmentationList.size() + 1);
}

QString SegmentMonitor::reduceSpkLabel(const QString &spkLbl)
{
  QRegularExpression re("^(.).+ (.+)$");
  QString newSpkLbl(spkLbl);

  QRegularExpressionMatch match = re.match(spkLbl);
    
  if (match.hasMatch())
    newSpkLbl = match.captured(1) + ". " + match.captured(2);
  
  return newSpkLbl;
}

void SegmentMonitor::positionChanged(qint64 position)
{
  m_currPosition = position;
  update();
}

void SegmentMonitor::updateDuration(qint64 duration)
{
  m_duration = duration;
  m_ratio = static_cast<qreal>(m_width) / m_duration;
  update();
}

void SegmentMonitor::setWidth(int newWidth)
{
  m_width = newWidth;
  resize(m_width, height());
  m_ratio = static_cast<qreal>(m_width) / m_duration;
  update();
}

void SegmentMonitor::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
  qreal x1, x2, x3, x4;
  qreal y1, y2, y3, y4;
  QString segLabel;
  QStringList interLoc;
  qint64 segStart;
  qint64 segEnd;
  QPainter painter(this);
  QColor color;
  QPen defaultPen(Qt::black);
  QPen shotSizePen(Qt::gray, 1, Qt::DotLine);
  QPen scenePen(Qt::lightGray, 1, Qt::DashDotDotLine);
  // QPen scenePen(Qt::black, 1);

  QFont font;
  font.setPointSize(m_segmentHeight * 2);
  painter.setFont(font);

  // filling background
  painter.fillRect(QRect(0, 0, m_width, height()), QBrush(Qt::white));

  // needed for drawing speech and music rate measurements
  int arMaxY = m_step * 2 - m_segmentHeight / 2;
  int arMinY = m_step + m_segmentHeight / 2;
  // QPointF srP1(0, arMaxY);
  QPointF mrP1(0, arMaxY);

  // drawing segments and associated information
  for (int i(0); i < m_segmentationList.size(); i++) {

    y1 = m_step * (i + 1) - m_segmentHeight / 2;
    y2 = y1 + m_segmentHeight;

    // surrounding each segmentation
    painter.setPen(Qt::lightGray);
    painter.drawRect(0, y1, m_width, m_segmentHeight);
    
    // speech rate estimate counters
    // int k(0);

    for (int j(0); j < m_segmentationList[i].size(); j++) {

      // retrieving segment features
      segLabel = m_segmentationList[i][j]->getLabel(m_source);

      if (i == 1)
	segLabel = m_segmentationList[i][j]->getLabel(Segment::Manual);

      interLoc = m_segmentationList[i][j]->getInterLocs(SpkInteractDialog::Ref);
      segStart = m_segmentationList[i][j]->getPosition();
      segEnd = m_segmentationList[i][j]->getEnd();
      color = m_segColors[segLabel];
      	
      if (m_selectedLabels[i] == "" || m_selectedLabels[i] == segLabel) {

	x1 = segStart * m_ratio;
	x2 = segEnd * m_ratio;
	x3 = (x1 + x2) / 2.0 - m_segmentHeight;
	x4 = (x1 + x2) / 2.0 + m_segmentHeight;
	
	if (x3 < x1)
	  x3 = x1;

	if (x4 > x2)
	  x4 = x2;

	y3 = (arMaxY + arMinY) / 2.0 - m_segmentHeight;
	y4 = (arMaxY + arMinY) / 2.0 + m_segmentHeight;

	// drawing segments labels
	if (m_selectedLabels[i] == segLabel) {
	  // if (i == 1) {
	  
	  // segLabel = QString::number(j);
	
	  QRectF rectF = painter.boundingRect(QRectF(x1, y2 + m_segmentHeight, x2 - x1, m_segmentHeight * 4), Qt::AlignHCenter, segLabel);
	  
	  if (rectF.width() <= x2 - x1) {
	    painter.setPen(defaultPen);
	    painter.drawText(rectF, Qt::AlignHCenter, segLabel);
	  }
	}

	// drawing shot sizes, scene boundaries, speech and music rates
	if (i == 0) {

	  // shot sizes
	  QRectF shotSize(QPointF(x1, y1 - m_shotSize[j] * y1), QPointF(x2, y1));
	  painter.fillRect(shotSize, QColor(220, 220, 220));
	  painter.setPen(shotSizePen);
	  painter.drawRect(shotSize);
	  
	  // scene boundaries
	  if (m_segmentationList[i][j]->row() == 0) {
	    painter.setPen(scenePen);
	    painter.drawLine(x1, 0, x1, height());
	  }

	  /*
	  // speech rate
	  qint64 t;
	  while (k < m_speechRate.size() && (t = m_speechRate[k].first) <= segEnd) {

	    qreal speechRate = m_speechRate[k].second;

	    int srX = qRound(t * m_ratio);
	    qreal y = arMaxY - (arMaxY - arMinY) * speechRate;

	    painter.setPen(Qt::lightGray);
	    
	    QPointF srP2(srX, y);

	    if (srP1.y() < arMaxY || srP2.y() < arMaxY)
	      painter.drawLine(srP1, srP2);

	    srP1 = srP2;

	    k++;
	  }
	  */

	  // music rate
	  Shot *shot = dynamic_cast<Shot *>(m_segmentationList[i][j]);
	  QList<QPair<qint64, qreal> > musicRates = shot->getMusicRates();

	  // enable antialiasing
	  painter.setRenderHint(QPainter::Antialiasing, true);
	  
	  for (int l(0); l < musicRates.size(); l++) {

	    qreal musicRate = musicRates[l].second;

	    int mrX = qRound(musicRates[l].first * m_ratio);
	    qreal y = arMaxY - (arMaxY - arMinY) * musicRate;

	    painter.setPen(Qt::lightGray);
	    
	    QPointF mrP2(mrX, y);

	    if (mrP1.y() < arMaxY || mrP2.y() < arMaxY)
	      painter.drawLine(mrP1, mrP2);

	    mrP1 = mrP2;
	  }

	  // disable antialiasing
	  painter.setRenderHint(QPainter::Antialiasing, false);
	}

	// drawing 
	painter.setPen(defaultPen);
	QPointF p1(x1, y1);
	QPointF p2(x2, y2);
	QPointF p3(x3, y3);
	QPointF p4(x4, y4);
	painter.fillRect(QRectF(p1, p2), color);
	painter.drawRect(QRectF(p1, p2));

	// draw and fill square indicating interlocutors
	/*
	if (i == 1) {
	  
	  if (interLoc.size() > 0) {
	    qreal width = (x4 - x3) / interLoc.size();

	    for (int k(0); k < interLoc.size(); k++) {
	      QPoint p5(x3 + k * width, y3);
	      QPoint p6(x3 + (k + 1) * width, y4);
	      QColor c = m_segColors[interLoc[k]];
	      painter.fillRect(QRectF(p5, p6), c);
	    }
	  }
	  
	  else
	    painter.drawRect(QRectF(p3, p4));
        }
	*/
      }
    }
  }

  // draw current position
  painter.setPen(Qt::gray);
  painter.drawLine(m_currPosition * m_ratio, 0, m_currPosition * m_ratio, height());
}

void SegmentMonitor::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {

    bool inBound(false);
    qreal inf, sup;
    int start(-1), end(-1);
  
    bool found(false);
    int min, max;
 
    qreal position(event->x() / m_ratio);

    // test if clicked in segmentation regions
    int i(0);
    int j(0);

    while (i < m_segmentationList.size() && !inBound) {
      inf = m_step * (i + 1);
      sup = inf + m_segmentHeight;
      if (event->y() >= inf && event->y() <= sup)
	inBound = true;
      i++;
    }

    // binary search to retrieve index of clicked segment
    if (inBound) {

      QList<Segment *> segList(m_segmentationList[--i]);
    
      min = 0;
      max = segList.size() - 1;
      
      while (min <= max && !found) {
	j = (min + max) / 2;
      
	start = segList[j]->getPosition();
	end = segList[j]->getEnd();

	if (position >= start && position <= end)
	  found = true;
	else if (position < start)
	  max = j - 1;
	else
	  min = j + 1;
      }
      
      if (found && (m_selectedLabels[i].isEmpty() || m_selectedLabels[i] == segList[j]->getLabel(m_source))) {
	QList<QPair<qint64, qint64> > utterances;
	utterances.push_back(QPair<qint64, qint64>(start, end));
	emit playSegments(utterances);
      }
    }
  }
}

void SegmentMonitor::mousePressEvent(QMouseEvent *event)
{
  bool inBound(false);
  qreal inf, sup;
  int start(-1), end(-1);
  
  bool found(false);
  int min, max;
 
  qreal position(event->x() / m_ratio);

  // test if clicked in segmentation regions
  int i(0);
  int j(0);

  while (i < m_segmentationList.size() && !inBound) {
    inf = m_step * (i + 1);
    sup = inf + m_segmentHeight;
    if (event->y() >= inf && event->y() <= sup)
      inBound = true;
    i++;
  }
    
  // binary search to retrieve index of segment clicked segment
  if (inBound) {

    QList<Segment *> segList(m_segmentationList[--i]);
    
    min = 0;
    min = 0;
    max = segList.size() - 1;
      
    while (min <= max && !found) {
      j = (min + max) / 2;
      
      start = segList[j]->getPosition();
      end = segList[j]->getEnd();

      if (position >= start && position <= end)
	found = true;
      else if (position < start)
	max = j - 1;
      else
	min = j + 1;
    }

    if (found && !m_annotMode && event->button() == Qt::RightButton) {
      QString segLabel = segList[j]->getLabel(m_source);
      if (segLabel != "C-1" && segLabel != "S" && m_selectedLabels[i].isEmpty())
	m_selectedLabels[i] = segLabel;
      else if (m_selectedLabels[i] == segList[j]->getLabel(m_source))
	m_selectedLabels[i] = "";
      update();
    }
  }
}

void SegmentMonitor::mouseMoveEvent(QMouseEvent *event)
{
  bool inBound(false);
  bool firstHalf(false);
  qreal inf, sup;
  int start(-1), end(-1);
  
  bool found(false);
  int min, max;
 
  qreal position(event->x() / m_ratio);

  // test if clicked in segmentation regions
  int i(0);
  int j(0);

  while (i < m_segmentationList.size() && !inBound) {
    inf = m_step * (i + 1);
    sup = inf + m_segmentHeight;
    if (event->y() >= inf && event->y() <= sup)
      inBound = true;
    i++;
  }
    
  // binary search to retrieve index of clicked segment
  if (i == 2 && inBound) {

    QList<Segment *> segList(m_segmentationList[--i]);
    
    min = 0;
    min = 0;
    max = segList.size() - 1;
      
    while (min <= max && !found) {
      j = (min + max) / 2;
      
      start = segList[j]->getPosition();
      
      // shot segmentation activated
      if (i == 0) {
	// last shot
	if (j == segList.size() - 1)
	  end = m_duration;
	else
	  end = segList[j+1]->getPosition();
      }

      // speech segmentation activated
      else
	end = segList[j]->getEnd();

      if (position >= start && position <= end)
	found = true;
      else if (position < start)
	max = j - 1;
      else
	min = j + 1;
    }

    // reference speech segment clicked for annotation purpose
    if (found && m_annotMode) {

      if (m_prevPosition != -1) {

	qint64 MIN_DUR(240);
	qint64 start, end, mid, position, newPosition, diff;
	qint64 prevSegStart(-1);
	qint64 prevSegEnd(-1);
	qint64 nextSegStart(-1);
	qint64 nextSegEnd(-1);
	int prevIdx = j - 1;
	int nextIdx = j + 1;

	start = segList[j]->getPosition();
	end = segList[j]->getEnd();
	mid = (start + end) / 2;

	// setting boundaries of surrounding segments
	if (prevIdx >= 0) {
	  prevSegStart = segList[prevIdx]->getPosition();
	  prevSegEnd = segList[prevIdx]->getEnd();
	}

	if (nextIdx < segList.size()) {
	  nextSegStart = segList[nextIdx]->getPosition();
	  nextSegEnd = segList[nextIdx]->getEnd();
	}

	// normalized mouse position in milliseconds from the beginning
	position = event->x() / m_ratio;

	if (m_prevPosition != -1)
	  diff = position - m_prevPosition;
	else
	  diff = 0;

	// modifying beginning of segment
	if (position >= start && position < mid) {

	  firstHalf = true;
	  newPosition = start + diff;
	  newPosition = static_cast<qint64>(ceil(newPosition / static_cast<qreal>(m_step)) * m_step);

	  // selected segment is not the first one
	  if (prevIdx > -1) {
	  
	    if (newPosition < prevSegEnd)
	      prevSegEnd = newPosition;
	    
	    if (end - newPosition >= MIN_DUR && prevSegEnd - prevSegStart >= MIN_DUR) {
	      segList[j]->setPosition(newPosition);
	      segList[prevIdx]->setEnd(prevSegEnd);
	    }
	  }

	  // selected segment is the first one
	  else if (end - newPosition >= MIN_DUR)
	    segList[j]->setPosition(newPosition);
	}
	 
	// modifying end of segment
	else if (position >= mid && position <= end) {

	  firstHalf = false;
	  newPosition = end + diff;
	  newPosition = static_cast<qint64>(floor(newPosition / static_cast<qreal>(m_step)) * m_step);

	  // selected segment is not the last one
	  if (nextIdx < segList.size()) {
	    
	    if (newPosition > nextSegStart)
	      nextSegStart = newPosition;
	    
	    if (newPosition - start >= MIN_DUR && nextSegEnd - nextSegStart >= MIN_DUR) {
	      segList[j]->setEnd(newPosition);
	      segList[nextIdx]->setPosition(nextSegStart);
	    }
	  }

	  // selected segment is the last one
	  else if (newPosition - start >= MIN_DUR)
	    segList[j]->setEnd(newPosition);
	}
	
	update();
      }
    }

    // updating normalized mouse position
    if (firstHalf)
      m_prevPosition = static_cast<qint64>(ceil(position / static_cast<qreal>(m_step)) * m_step);
    else
      m_prevPosition = static_cast<qint64>(floor(position / static_cast<qreal>(m_step)) * m_step);
  }
}

void SegmentMonitor::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
  m_prevPosition = -1;
}

void SegmentMonitor::showContextMenu(const QPoint &point)
{
  // retrieving selected speech segment
  bool inSpeechBound(false);
  bool inInterLocBound(false);
  qreal inf, sup;
  int start(-1), end(-1);
  
  bool found(false);
  int min, max;
 
  qreal position(point.x() / m_ratio);

  // test if clicked in segmentation regions
  int i(0);
  int j(0);

  while (i < m_segmentationList.size() && !inSpeechBound) {
    inf = m_step * (i + 1);
    sup = inf + m_segmentHeight;
    if (point.y() >= inf && point.y() <= sup)
      inSpeechBound = true;
    i++;
  }

  int arMaxY = m_step * 2 - m_segmentHeight / 2;
  int arMinY = m_step + m_segmentHeight / 2;

  inf = (arMaxY + arMinY) / 2.0 - m_segmentHeight;
  sup = (arMaxY + arMinY) / 2.0 + m_segmentHeight;

  if (point.y() >= inf && point.y() <= sup)
    inInterLocBound = true;

  // binary search to retrieve index of clicked segment
  if (i == 2 && (inSpeechBound || inInterLocBound)) {

    QList<Segment *> segList(m_segmentationList[--i]);
    
    min = 0;
    max = segList.size() - 1;
      
    while (min <= max && !found) {
      j = (min + max) / 2;
      
      if (inSpeechBound) {
	start = segList[j]->getPosition();
	end = segList[j]->getEnd();
      }

      else if (inInterLocBound) {
	start = (segList[j]->getPosition() + segList[j]->getEnd()) / 2.0 - m_segmentHeight / m_ratio;
	end = (segList[j]->getPosition() + segList[j]->getEnd()) / 2.0 + m_segmentHeight / m_ratio;
      }
      
      if (position >= start && position <= end)
	found = true;
      else if (position < start)
	max = j - 1;
      else
	min = j + 1;
    }

    if (found && m_annotMode) {
      QPoint globalPos = mapToGlobal(point);
      QAction *selected = m_speakers->exec(globalPos, m_sep);

      if (selected && selected != m_newSpeakerAct && selected != m_anonymizeAct && selected != m_removeAct && selected != m_insertAct && selected != m_splitAct) {

	if (inSpeechBound)
	  segList[j]->setLabel(selected->text(), m_source);
	else if (selected->text() != segList[j]->getLabel(m_source))
	  segList[j]->addInterLoc(selected->text(), SpkInteractDialog::Ref);

	m_speakers->removeAction(selected);
	m_speakers->insertAction(m_sep, selected);
	
	update();
      }
      
      else if (selected && selected == m_anonymizeAct) {

	if (inSpeechBound)
	  segList[j]->setLabel("S", m_source);
	else
	  segList[j]->resetInterLocs(SpkInteractDialog::Ref);

	update();
      }

      else if (selected && selected == m_removeAct) {
	if (inSpeechBound)
	  m_segmentationList[i].removeAt(j);
	
	update();
      }

      else if (selected && selected == m_splitAct) {
	if (inSpeechBound) {
	  qint64 start = segList[j]->getPosition();
	  qint64 end = segList[j]->getEnd();
	  qint64 med = static_cast<qint64>((start + end) / 2);

	  SpeechSegment *currSpeechSegment = dynamic_cast<SpeechSegment *>(segList[j]);
	  SpeechSegment *newSpeechSegment = new SpeechSegment(start, med, currSpeechSegment->getText(), currSpeechSegment->getSpeakers(), currSpeechSegment->getInterLocs(), currSpeechSegment->parent(), m_source);
	  newSpeechSegment->setLabel("S", m_source);
	  segList[j]->setPosition(med);
	  m_segmentationList[i].insert(j, newSpeechSegment);
	}
	  
	update();
      }
    }
  }
}

void SegmentMonitor::addNewSpeaker()
{
  bool ok(false);
  QString speaker = QInputDialog::getText(this, "New speaker", "New speaker name:", QLineEdit::Normal, QString(), &ok);

  speaker = processSpkLabel(speaker);

  if (ok && !speaker.isEmpty()) {
    m_speakers->insertAction(m_sep, new QAction(speaker, this));
    m_segColors[speaker] = m_colors[m_idxColor++ % m_colors.size()];
  }
}

/////////////////////
// private methods //
/////////////////////

QString SegmentMonitor::processSpkLabel(QString spkLabel)
{
  // regular expression to detext spaces at beginning/end of speaker label
  QRegularExpression spacesLeft("^\\s+");
  QRegularExpression spacesRight("\\s+$");
  QRegularExpression spacesMid("\\s+");

  QRegularExpressionMatch spacesLeftMatch = spacesLeft.match(spkLabel);
  if (spacesLeftMatch.hasMatch())
    spkLabel.replace(spacesLeft, "");

  QRegularExpressionMatch spacesRightMatch = spacesRight.match(spkLabel);
  if (spacesRightMatch.hasMatch())
    spkLabel.replace(spacesRight, "");

  QRegularExpressionMatch spacesMidMatch = spacesMid.match(spkLabel);
  if (spacesMidMatch.hasMatch())
    spkLabel.replace(spacesMid, " ");

  return spkLabel;
}

void SegmentMonitor::computeSpeechRate(QList<Segment *> speechSegments, qint64 end)
{
  const int timeStep(500);                          // time interval between two measurements
  const int deltaT(1500);                           // time step used to estimate speech rate
  QList<QPair<qint64, int> > cumNbUnits;            // total number of uttered units as a function of time
  TextProcessor *textProcessor = new TextProcessor; // used to count words

  int totNbUnits(0);
  for (int i(0); i < speechSegments.size(); i++) {

    // current speech segments
    SpeechSegment *currSpeechSeg = dynamic_cast<SpeechSegment *>(speechSegments[i]);

    // speech segment textual content
    QString text = currSpeechSeg->getText();
    text = textProcessor->preProcess(text);

    // words contained in current speech segment
    QStringList words = text.split(" ");

    // segment duration
    qint64 start = currSpeechSeg->getPosition();
    qint64 end = currSpeechSeg->getEnd();
    
    // retrieving word timestamps
    QList<qint64> charTimeStamps = getCharTimeStamps(words, start, end);

    // updating word count
    for (int i(0); i < charTimeStamps.size(); i++)
      cumNbUnits.push_back(QPair<qint64, int>(charTimeStamps[i], ++totNbUnits));
  }

  int i(0);
  qint64 time(0);
  qreal maxSpeechRate(0.0);
  while (i < cumNbUnits.size()) {

    // retrieve number of words uttered within current time interval [time - deltaT / 2, time + deltaT / 2]
   
    qint64 interStart = time - deltaT / 2;
    if (interStart < 0)
      interStart = 0;
    qint64 interEnd = time + deltaT / 2;
    if (interEnd > end)
      interEnd = end;
   
    int interNbUnits(0);
    qreal speechRate(0.0);

    while (i >= 0 && cumNbUnits[i].first >= interStart)
      i--;

    if (i < 0)
      i = 0;

    while (i < cumNbUnits.size() && cumNbUnits[i].first < interStart)
      i++;

    int j(i);
    while (j < cumNbUnits.size() && cumNbUnits[j].first <= interEnd)
      j++;
    j--;
    
    // time interval contains words
    if (j >= i)
      interNbUnits = cumNbUnits[j].second - cumNbUnits[i].second + 1;

    speechRate = interNbUnits * 1000.0 / deltaT;
    if (speechRate > maxSpeechRate)
      maxSpeechRate = speechRate;
    m_speechRate.push_back(QPair<qint64, qreal>(time, speechRate));

    // qDebug() << interStart << interEnd << interNbUnits << speechRate;
    time += timeStep;
  }

  // normalizing speech rate
  for (int i(0); i < m_speechRate.size(); i++)
    m_speechRate[i].second /= maxSpeechRate;
}

QList<qint64> SegmentMonitor::getCharTimeStamps(const QStringList &words, qint64 start, qint64 end)
{
  QList<qint64> charTimeStamps;
  int duration = end - start;

  // retrieve number of characters contained in current speech segment
  int nChar(0);
  for (int i(0); i < words.size(); i++)
    nChar += words[i].size();
  
  // assignig timestamp to each character
  qreal charDur = duration / static_cast<qreal>(nChar);
  for (int i(0); i < nChar; i++)
    charTimeStamps.push_back(start + static_cast<int>(i * charDur));

  return charTimeStamps;
}
