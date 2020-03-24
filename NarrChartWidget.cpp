#include <QPainter>
#include <QtMath>

#include <QDebug>

#include "NarrChartWidget.h"

NarrChartWidget::NarrChartWidget(QWidget *parent)
  : QWidget(parent),
    m_refX(0),
    m_refY(0),
    m_firstScene(0),
    m_lastScene(0),
    m_minHeight(0),
    m_maxHeight(0),
    m_sceneWidth(5),
    m_vertSpace(8),
    m_posFixed(false),
    m_selStoryLine(""),
    m_dispLines(false),
    m_dispArcs(false)
{
  setMinimumSize(1400, 500);
  setMouseTracking(true);
}

void NarrChartWidget::setNarrChart(const QMap<QString, QVector<QPoint> > &narrChart)
{
  m_narrChart = narrChart;

  // set of colors
  QList<QColor> colors;

  colors << QColor(219, 213, 185) << QColor(224, 199, 30);
  colors << QColor(186, 22, 63) << QColor(255, 250, 129);
  colors << QColor(181, 225, 174) << QColor(154, 206, 223);
  colors << QColor(249, 149, 51) << QColor(252, 169, 133);
  colors << QColor(193, 179, 215) << QColor(116, 161, 97);
  colors << QColor(133, 202, 93) << QColor(72, 181, 163);
  colors << QColor(69, 114, 147) << QColor(249, 150, 182);
  colors << QColor(117, 137, 191) << QColor(84, 102, 84);

  // set story-lines colors
  QMap<QString, QVector<QPoint> >::const_iterator it = m_narrChart.begin();

  int i(0);
  while (it != m_narrChart.end()) {

    QString speaker = it.key();
    m_storyLineColors[speaker] = colors[i % colors.size()];
      
    it++;
    i++;
  }

  // retrieve last scene index and max height
  m_lastScene = 0;
  m_maxHeight = 0;

  it = m_narrChart.begin();
  while (it != m_narrChart.end()) {

    QVector<QPoint> points = it.value();

    for (int i(0); i < points.size(); i++) {
      if (points[i].x() > m_lastScene)
	m_lastScene = points[i].x();
      
      if (points[i].y() > m_maxHeight)
	m_maxHeight = points[i].y();
    }

    it++;
  }

  // retrieve first scene index and min height
  m_firstScene = m_lastScene;
  m_minHeight = m_maxHeight;

  it = m_narrChart.begin();
  while (it != m_narrChart.end()) {

    QVector<QPoint> points = it.value();

    for (int i(0); i < points.size(); i++) {
      if (points[i].x() < m_firstScene)
	m_firstScene = points[i].x();

      if (points[i].y() < m_minHeight)
	m_minHeight = points[i].y();
    }

    it++;
  }

  // retrieve character name of maximum length
  QFont textFont;
  int fontSize = 10;
  textFont.setWeight(63);
  textFont.setPixelSize(fontSize);

  it = m_narrChart.begin();
  
  while (it != m_narrChart.end()) {

    QString speaker = it.key();
    
    QFontMetrics fm(textFont);
    int labelWidth = fm.width(speaker);

    if (labelWidth > m_refX)
      m_refX = labelWidth;

    it++;
  }

  m_refX += 8;
  m_refY = m_vertSpace * 5;

  setFixedSize((m_lastScene - m_firstScene + 1) * m_sceneWidth + m_refX * 2,
	       (m_maxHeight - m_minHeight + 10) * m_vertSpace);
}

void NarrChartWidget::setSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots)
{
  m_snapshots = snapshots;
}

void NarrChartWidget::setSceneRefs(const QVector<QPair<int, int> > &sceneRefs)
{
  m_sceneRefs = sceneRefs;
}

void NarrChartWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  // font parameters
  QFont textFont;
  int fontSize = 10;
  textFont.setWeight(63);
  textFont.setPixelSize(fontSize);

  // line parameters
  int lineWidth(1);
  QPen linePen(QColor::fromRgb(180, 180, 180, 255), lineWidth, Qt::DotLine);

  // story-lines parameters
  int storyLineWidth(2);
  QPen storyLinePen(Qt::white, storyLineWidth);

  QPainter painter(this);

  // filling background
  int fGrayLevel(60);
  int sGrayLevel(70);

  painter.fillRect(QRect(0, 0, width(), height()), QColor::fromRgb(fGrayLevel, fGrayLevel, fGrayLevel, 255));

  int nLines = m_lastScene - m_firstScene + 1;

  for (int i(0); i < nLines; i++) {
    
    int grayLevel = sGrayLevel;

    if (m_sceneRefs[i].first % 2 == 1)
      grayLevel = fGrayLevel;

    painter.fillRect(QRect(m_refX + i * m_sceneWidth + m_sceneWidth / 4.0, 0, m_sceneWidth, height()), QColor::fromRgb(grayLevel, grayLevel, grayLevel, 255));
  }

  if (m_dispLines) {

    // draw horizontal lines
    painter.setPen(linePen);

    /*
    nLines = m_maxHeight - m_minHeight + 1;

    for (int i(0); i < nLines; i++) {
      int x1 = m_refX;
      int x2 = m_refX + (m_lastScene - m_firstScene + 1) * m_sceneWidth;
      int y = m_refY + i * m_vertSpace - lineWidth / 2.0;
      painter.drawLine(x1, y, x2, y);
    }
    */
    
    // draw vertical lines and scene labels
    painter.setFont(textFont);
    nLines = m_lastScene - m_firstScene + 1;

    for (int i(0); i < nLines; i++) {

      int y1 = m_refY - m_vertSpace;
      int y2 = m_refY + m_vertSpace * (m_maxHeight - m_minHeight + 1);
      int x = m_refX + i * m_sceneWidth + m_sceneWidth / 2.0;
      painter.drawLine(x, y1, x, y2);

      QString label = QString::number(i + m_firstScene);
      QFontMetrics fm(textFont);
      int labelWidth = fm.width(label);
      painter.drawText(x - labelWidth / 2.0, y1 - 5, label);
      painter.drawText(x - labelWidth / 2.0, y2 + fontSize + 5, label);
    }
  }

  // set story-line weights
  QVector<QMap<qreal, QStringList> > storyLineWeights(m_lastScene - m_firstScene + 1);
  QMap<QString, QVector<QPoint> >::const_iterator it = m_narrChart.begin();

  while (it != m_narrChart.end()) {

    QString speaker = it.key();
    QVector<QPoint> points = it.value();

    for (int i(0); i < points.size(); i++) {

      // set story-line weight
      qreal weight(1.0);
     
      if (m_selStoryLine != "" && speaker != m_selStoryLine) {
	
	QString fSpk = (speaker < m_selStoryLine) ? speaker : m_selStoryLine;
	QString sSpk = (speaker > m_selStoryLine) ? speaker : m_selStoryLine;

	weight = m_snapshots[points[i].x()][fSpk][sSpk];
      }

      storyLineWeights[i][weight].push_back(speaker);
    }
    it++;
  }

  // draw story-lines
  for (int i(0); i < storyLineWeights.size(); i++) {

    QMap<qreal, QStringList>::const_iterator it = storyLineWeights[i].begin();

    int grayLevel = sGrayLevel;

    if (m_sceneRefs[i].first % 2 == 1)
      grayLevel = fGrayLevel;

    while (it != storyLineWeights[i].end()) {

      qreal weight = it.key();
      QStringList speakers = it.value();

      for (int j(0); j < speakers.size(); j++) {

	QString speaker = speakers[j];
 
	// set coordinates
	QPoint p1 = m_narrChart[speaker][i];
     
	int x1 = m_refX + (p1.x() - m_firstScene) * m_sceneWidth + m_sceneWidth / 4.0;
	if (i == 0)
	  x1 -= m_sceneWidth / 4.0;
	int x2 = m_refX + (p1.x() - m_firstScene) * m_sceneWidth + 3 * m_sceneWidth / 4.0;
	if (i == storyLineWeights.size() - 1)
	  x2 += m_sceneWidth / 4.0;

	int y = m_refY + p1.y() * m_vertSpace - storyLineWidth / 2.0;

	// set color
	if (m_selStoryLine != "")
	  storyLinePen.setColor(getRGBColor(m_storyLineColors[m_selStoryLine], weight, grayLevel));
	else
	  storyLinePen.setColor(getRGBColor(m_storyLineColors[speaker], weight, grayLevel));
	painter.setPen(storyLinePen);

	// drawing
	if (m_selStoryLine == "" || weight > 0.0) {

	  // enable antialiasing
	  painter.setRenderHint(QPainter::Antialiasing, true);

	  // draw story-line
	  QPainterPath path;
	  path.moveTo(x1, y);
	  path.lineTo(x2, y);

	  // possibly join next scene
	  if (i < storyLineWeights.size() - 1) {
	    QPoint p2 = m_narrChart[speaker][i+1];
	    int x3 = m_refX + (p2.x() - m_firstScene) * m_sceneWidth + m_sceneWidth / 4;
	    int y3 = m_refY + p2.y() * m_vertSpace - storyLineWidth / 2.0;
	    path.cubicTo(x2 + m_sceneWidth / 4.0, y, x3 - m_sceneWidth / 4.0, y3, x3, y3);
	  }

	  painter.drawPath(path);

	  // possibly draw arc to reference speaker
	  if (m_dispArcs && m_selStoryLine != "" && weight > 0.0) {

	    // set coordinates
	    QPoint p2 = m_narrChart[m_selStoryLine][i];
     	    int y2 = m_refY + p2.y() * m_vertSpace - storyLineWidth / 2.0;
	    int y3 = (y < y2) ? y : y2;

	    QRectF rectangle(x1 - m_sceneWidth / 4.0, y3, m_sceneWidth, qAbs(y2 - y));
	    int startAngle = 90 * 16;
	    int spanAngle = 180 * 16;

	    painter.drawArc(rectangle, startAngle, spanAngle);
	  }

	  // draw character selected in fixed mode
	  if (m_posFixed &&
	      m_pos.x() >= x1 - m_sceneWidth / 4.0 &&
	      m_pos.x() <= x2 +  m_sceneWidth / 4.0 &&
	      m_pos.y() >= y - m_vertSpace / 2.0 &&
	      m_pos.y() <= y + m_vertSpace / 2.0) {
	    
	    // disable antialiasing
	    painter.setRenderHint(QPainter::Antialiasing, false);

	    // display character label
	    painter.drawText(m_pos.x(), m_pos.y() - m_vertSpace / 4.0, speaker);
	  }
	}

	// display character label
	if (i == 0) {
	
	  // disable antialiasing
	  painter.setRenderHint(QPainter::Antialiasing, false);

	  painter.setFont(textFont);
	  QFontMetrics fm(textFont);
	  int labelWidth = fm.width(speaker);
	  painter.drawText(m_refX - labelWidth - 4, y + fontSize / 3.0, speaker);
	}
	
	else if (i == storyLineWeights.size() - 1) {

	  // disable antialiasing
	  painter.setRenderHint(QPainter::Antialiasing, false);

	  painter.setFont(textFont);
	  int x = m_refX + (p1.x() - m_firstScene + 1) * m_sceneWidth + 4;
	  painter.drawText(x, y + fontSize / 3.0, speaker);
	}
      }

      it++;
    }
  }

  // display character in case of story-line selection
  if (!m_posFixed && m_selStoryLine != "") {

    // disable antialiasing
    painter.setRenderHint(QPainter::Antialiasing, false);

    storyLinePen.setColor(m_storyLineColors[m_selStoryLine]);
    painter.setPen(storyLinePen);
    
    painter.drawText(m_pos.x(), m_pos.y() - m_vertSpace / 4.0, m_selStoryLine);
  }
}

QSize NarrChartWidget::minimumSizeHint() const
{
  return QSize(1400, 500);
}

QSize NarrChartWidget::sizeHint() const
{
  return QSize(1400, 500);
}

void NarrChartWidget::mouseMoveEvent(QMouseEvent *event)
{
  m_pos = event->pos();
  if (!m_posFixed)
    m_selStoryLine = storyLineSelected(event);
  update();
}

void NarrChartWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  Q_UNUSED(event);

  QString selStoryLine = storyLineSelected(event);
  
  if (selStoryLine == m_selStoryLine) {
    m_posFixed = !m_posFixed;
    update();
  }
}

QString NarrChartWidget::storyLineSelected(QMouseEvent *event)
{
  QString selStoryLine("");

  qreal x = (event->x() - m_refX - m_sceneWidth / 2.0) / m_sceneWidth + m_firstScene;
  qreal y = (event->y() - m_refY) / static_cast<qreal>(m_vertSpace);

  QPoint pos(qRound(x), qRound(y));

  QMap<QString, QVector<QPoint> >::const_iterator it = m_narrChart.begin();
  
  while (it != m_narrChart.end() && selStoryLine == "") {

    QString speaker = it.key();
    QVector<QPoint> points = it.value();

    for (int i(0); i < points.size(); i++)
      if (points[i] == pos) {
	selStoryLine = speaker;
	break;
      }
    
    it++;
  }

  return selStoryLine;
}

 QColor NarrChartWidget::getRGBColor(const QColor &refColor, qreal weight, int grayLevel)
{
  QColor color;
  qreal h, s, v, hsvA;
  qreal normFac(1.0);

  // retrieving hue, saturation and value components of reference color
  refColor.getHsvF(&h, &s, &v, &hsvA);

  // apply non-linear scale to weight
  weight = qPow(weight, 1.0 / normFac);

  // computing saturation and value components for current vertex/edge
  // as a proportion of reference components
  s *= weight;
  v *= weight * (1.0 - grayLevel / 255.0);
  v += grayLevel / 255.0;

  // color to be returned
  color = QColor::fromHsvF(h, s, v, hsvA);

  return color;
}

///////////////
// accessors //
///////////////

QPair<int, int> NarrChartWidget::getSceneBound() const
{
  return QPair<int, int>(m_firstScene, m_lastScene);
}

int NarrChartWidget::getXCoord(int iScene) const
{
  return m_refX + (iScene - m_firstScene) * m_sceneWidth + m_sceneWidth / 2.0;
}

///////////
// slots //
///////////

void NarrChartWidget::displayLines(bool dispLines)
{
  m_dispLines = dispLines;
  update();
}

void NarrChartWidget::displayArcs(bool dispArcs)
{
  m_dispArcs = dispArcs;
  update();
}
