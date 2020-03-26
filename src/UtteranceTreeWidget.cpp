#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

#include "UtteranceTreeWidget.h"

using namespace arma;

UtteranceTreeWidget::UtteranceTreeWidget(int width, int height, QWidget *parent)
  : QWidget(parent),
    m_width(width),
    m_height(height),
    m_yPad(0.0),
    m_YRatio(1.0),
    m_posReleased(true)
{
  setFixedSize(m_width, m_height);
  setMouseTracking(true);

  m_tree = new UtteranceTree;
}

QList<QList<int>> UtteranceTreeWidget::getPartition()
{
  return m_tree->getPartition();
}

///////////
// slots //
///////////

void UtteranceTreeWidget::setDiff(const arma::mat &Diff)
{
  m_tree->setDiff(Diff);
  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  // m_tree->displayTree(m_map);

  update();
}

void UtteranceTreeWidget::updateUtteranceTree(const mat &S, const mat &W, const umat &map, const mat &SigmaInv)
{
  m_S = S;
  m_unnormS = S;
  m_W = W;
  m_SigmaInv = SigmaInv;
  m_map = map;
  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  // m_tree->displayTree(m_map);
  
  update();
}

void UtteranceTreeWidget::setDistance(UtteranceTree::DistType dist)
{
  m_tree->setDist(dist);
  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  // m_tree->displayTree(m_map);
  
  update();
}

void UtteranceTreeWidget::setCovInv(const mat &CovInv)
{
  m_SigmaInv = CovInv;
  
  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  // m_tree->displayTree(m_map);

  update();
}

void UtteranceTreeWidget::setAgrCrit(UtteranceTree::AgrCrit agr)
{
  m_tree->setAgr(agr);
  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  // m_tree->displayTree(m_map);

  update();
}

void UtteranceTreeWidget::setWeight(const mat &W)
{
  m_W = W;
  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  // m_tree->displayTree(m_map);

  update();
}

void UtteranceTreeWidget::setPartitionMethod(UtteranceTree::PartMeth partMeth)
{
  m_tree->setPartMeth(partMeth);
  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  // m_tree->displayTree(m_map);

  update();
}

void UtteranceTreeWidget::normalizeVectors(bool checked)
{
  UtteranceTree::DistType distType = m_tree->getDist();
  qreal normFac;

  for (uword i(0); i < m_unnormS.n_rows; i++) {
    switch (distType) {

    case UtteranceTree::L2:
      normFac = as_scalar(m_unnormS.row(i) * m_unnormS.row(i).t());
      break;

    case UtteranceTree::Mahal:
      normFac = as_scalar(m_unnormS.row(i) * m_SigmaInv * m_unnormS.row(i).t());
      break;
    }

    if (checked)
      m_S.row(i) = m_unnormS.row(i) / normFac;
    else
      m_S.row(i) = m_unnormS.row(i);
  }

  m_tree->setTree(m_S, m_W, m_SigmaInv);
  m_cutDist = m_tree->getCutValues();

  update();
}

void UtteranceTreeWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  
  qreal x1, x2, y1, y2;                    // node coordinates
  qreal refWidth = 3.0 / 4.0 * m_width;    // width available for drawing
  qreal refHeight = 3.0 / 4.0 * m_height;  // height available for drawing
  qreal xPad = (m_width - refWidth) / 2;   // shift along X-axis
  m_yPad = (m_height - refHeight) / 2;     // shift along Y-axis
  int subRef;                              // subtitle index
  QString subLabel;                        // and its string version
  QFont font;
  int pixelSize(10);
  QString gradLabel;
  QList<QPoint> selectedNodes;
  qreal lineWidth(2.0);

  // font settings
  font.setBold(true);
  font.setPixelSize(pixelSize);
  painter.setFont(font);
  painter.setPen(QPen(QBrush(Qt::black), lineWidth));

  // filling background
  painter.fillRect(QRect(0, 0, m_width, m_height), QBrush(Qt::white));

  // retrieving nodes coordinates
  QList<QPair<QLineF, QPair<int, bool>>> coord;
  m_tree->getCoordinates(coord);
  
  // removing first line
  if (coord.size() > 0)
    coord.pop_front();

  // number of instances
  qreal maxWidth(m_tree->getClusterSize()-1);

  // draw tree corresponding to possible partitions of the data set
  if (maxWidth > 0) {

    // horizontal and vertical scaling factors
    qreal XRatio(refWidth / maxWidth);
    qreal maxHeight(coord[0].first.y1());
    m_YRatio = refHeight / maxHeight;

    // draw root
    if (coord[0].second.second) {
      x1 = xPad + coord[0].first.x1() * XRatio;
      y1 = m_height - (m_yPad + coord[0].first.y1() * m_YRatio);
      painter.drawLine(x1, 0, x1, y1);
    }

    // drawing y-axis
    x1 = 1;
    x2 = m_width - 1;
    y1 = m_yPad;
    y2 = m_height - m_yPad;;
    painter.drawLine(x1, y1, x1, y2);
    painter.drawLine(x2, y1, x2, y2);

    // drawing gradations
    painter.setPen(QPen(QBrush(Qt::black), lineWidth));
    const int nGrad(3);
    const int gradWidth(4);

    for (int i(0); i <= nGrad; i++) {
      gradLabel = QString::number(i * maxHeight / nGrad, 'g', 3);

      x1 = 1;
      x2 = 1 + gradWidth;
      y1 = m_height - (m_yPad + i * refHeight / nGrad);
      painter.drawLine(x1, y1, x2, y1);
      painter.drawText(QRectF(x2 + 2, y1 - 7, xPad, m_yPad), Qt::AlignLeft, gradLabel);

      x1 = m_width - 1;
      x2 = m_width - (1 + gradWidth);
      painter.drawLine(x1, y1, x2, y1);
      painter.drawText(QRectF(x2 - (xPad + 2), y1 - 7, xPad, m_yPad), Qt::AlignRight, gradLabel);
    }

    // drawing tree(s)
    m_selectedUtter.clear();

    QVector<QPair<qreal, qreal>> treeBound(m_cutDist.size());
    for (int i(0); i < treeBound.size(); i++)
      treeBound[i] = QPair<qreal, qreal>(width(), 0.0);

    QList<QList<QPointF>> treesCoord;
    QList<QPointF> invisPoints;
    
    for (int i(0); i < coord.size(); i++) {

      // setting drawing color
      QColor color;
      bool visible = coord[i].second.second;
      color = Qt::black;

      // coordinates required to draw from parent node
      x1 = xPad + coord[i].first.x1() * XRatio;
      x2 = xPad + coord[i].first.x2() * XRatio;
      y1 = m_height - (m_yPad + coord[i].first.y1() * m_YRatio);
      y2 = m_height - (m_yPad + coord[i].first.y2() * m_YRatio);

      // adding root coordinates to dendogram lists of points
      if (coord[i].first.y1() == coord[0].first.y1()) {

	QList<QPointF> pointList;
	
	// case of single dendogram (unconstrained clustering)
	if (m_cutDist.size() == 1) {
	  pointList << QPointF(x1, y1);
	  if (treesCoord.isEmpty())
	    treesCoord.push_back(pointList);
	  else
	    treesCoord[0].push_back(QPointF(x2, y2));
	}

	// case of multiple dendograms (constrained clustering)
	else if (y2 > y1) {
	  pointList << QPointF(x2, y2);
	  treesCoord.push_back(pointList);
	}
      }

      // updating list of points for each dendogram
      int j(0);
      while (j < treesCoord.size() && !treesCoord[j].contains(QPointF(x1, y1)))
	j++;

      if (j < treesCoord.size()) {
	treesCoord[j].push_back(QPointF(x2, y2));

	// adjusting dendogram boundaries
	qreal lim =  m_height - (m_yPad + m_cutDist[j] * m_YRatio);

	if (x2 < treeBound[j].first && y1 < lim && y2 > lim)
	  treeBound[j].first = x2;
	if (x2 > treeBound[j].second && y1 < lim && y2 > lim)
	  treeBound[j].second = x2 + lineWidth;
      }

      // node label
      subRef = coord[i].second.first;

      // change color if mouse points to current node
      if ((((m_mousePos.x() <= x1 && m_mousePos.x() >= x2) ||
	    (m_mousePos.x() >= x1 && m_mousePos.x() <= x2)) &&
	   (m_mousePos.y() > y1 && m_mousePos.y() <= y2)) || 
	  selectedNodes.contains(QPoint(x1, y1))) {
	color = Qt::red;
	selectedNodes.push_back(QPoint(x2, y2));

	if (subRef != -1)
	  m_selectedUtter.push_back(as_scalar(m_map.col(subRef)));
      }

      // drawing from previous node
      if (!visible) {
	invisPoints.push_back(QPointF(x2, y2));
	color = Qt::white;
      }
      
      else if (invisPoints.contains(QPointF(x1, y1))) {
	painter.setPen(QPen(QBrush(color), lineWidth));
	painter.drawLine(x1, y1 - gradWidth * 4, x1, y1);
      }

      painter.setPen(QPen(QBrush(color), lineWidth));
      painter.drawLine(x1, y1, x2, y1);
      painter.drawLine(x2, y1, x2, y2);
    }

    // drawing optimal cut(s)
    painter.setPen(QPen(QBrush(Qt::black), lineWidth / 2, Qt::DotLine));
    for (int i(0); i < m_cutDist.size(); i++) {
      y1 = m_height - (m_yPad + m_cutDist[i] * m_YRatio);
      painter.drawLine(treeBound[i].first - lineWidth * 3, y1, treeBound[i].second + lineWidth * 2, y1);
    }

    qSort(m_selectedUtter);
    emit(setSpeakers(m_selectedUtter));
  }
}

void UtteranceTreeWidget::mousePressEvent(QMouseEvent * event)
{
  Q_UNUSED(event);

  m_posReleased = !m_posReleased;
}

void UtteranceTreeWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
  Q_UNUSED(event);

  if (!m_selectedUtter.isEmpty())
    emit playSubtitle(m_selectedUtter);
}

void UtteranceTreeWidget::mouseMoveEvent(QMouseEvent * event)
{
  if (m_posReleased) {
    m_mousePos.setX(event->x());
    m_mousePos.setY(event->y());
    update();
  }
}

void UtteranceTreeWidget::setCurrSubtitle(int subIdx)
{
  QString currSub = QString::number(subIdx+1);
  
  if (currSub != m_currSub) {
    m_currSub = currSub;
    update();
  }
}

void UtteranceTreeWidget::releasePos(bool released)
{
  m_posReleased = released;
}
