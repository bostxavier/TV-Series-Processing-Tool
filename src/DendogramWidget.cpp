#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

#include "DendogramWidget.h"

using namespace std;

DendogramWidget::DendogramWidget(const QString &speaker, Dendogram *dendogram, const QList<arma::vec> &instances,const QList<QPair<QString, QString> > &vecComponents, QWidget *parent)
  : QWidget(parent),
    m_speaker(speaker),
    m_dendogram(dendogram),
    m_instances(instances),
    m_vecComponents(vecComponents)    ,
    m_iMed(-2)
{
  setWindowTitle(speaker);
  setMinimumSize(600, 400);
  setMouseTracking(true);
}

void DendogramWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);

  qreal x1, x2, y1, y2;                    // node coordinates
  qreal refWidth = 3.0 / 4.0 * width();    // width available for drawing
  qreal refHeight = 3.0 / 4.0 * height();  // height available for drawing
  qreal xPad = (width() - refWidth) / 2;   // shift along X-axis
  qreal yPad = (height() - refHeight) / 2; // shift along Y-axis

  QColor refColor = QColor::fromRgb(178, 34, 34, 255);
  QFont font;
  int pixelSize(10);
  int labelPixelSize(9);
  int spkPixelSize(12);
  QString gradLabel;
  QList<QPoint> selectedNodes;
  QList<int> selectedLabels;
  qreal lineWidth(2.0);

  // font settings
  font.setBold(true);
  font.setPixelSize(pixelSize);
  painter.setFont(font);
  painter.setPen(QPen(QBrush(Qt::black), lineWidth));

  // filling background
  painter.fillRect(QRect(0, 0, width(), height()), QBrush(Qt::white));

  // retrieving nodes coordinates
  QList<QPair<QLineF, QPair<int, bool> > > coord;
  m_dendogram->getCoordinates(coord);

  // removing first line
  if (coord.size() > 0)
    coord.pop_front();

  // number of instances
  qreal maxWidth(m_dendogram->getClusterSize() - 1);

  // horizontal and vertical scaling factors
  qreal XRatio(refWidth / maxWidth);
  qreal maxHeight(coord[0].first.y1());
  qreal YRatio = refHeight / maxHeight;

  // draw root
  if (coord[0].second.second) {
    x1 = xPad + coord[0].first.x1() * XRatio;
    y1 = height() - (yPad + coord[0].first.y1() * YRatio);
    painter.drawLine(x1, 0, x1, y1);
  }

  // drawing y-axis
  x1 = 1;
  x2 = width() - 1;
  y1 = yPad;
  y2 = height() - yPad;;
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
    y1 = height() - (yPad + i * refHeight / nGrad);
    painter.drawLine(x1, y1, x2, y1);
    painter.drawText(QRectF(x2 + 2, y1 - 7, xPad, yPad), Qt::AlignLeft, gradLabel);

    x1 = width() - 1;
    x2 = width() - (1 + gradWidth);
    painter.drawLine(x1, y1, x2, y1);
    painter.drawText(QRectF(x2 - (xPad + 2), y1 - 7, xPad, yPad), Qt::AlignRight, gradLabel);
  }

  // drawing tree
  for (int i(0); i < coord.size(); i++) {

    // setting drawing color
    QColor color = Qt::black;
    
    // current node id
    int nodeId = coord[i].second.first;

    // coordinates required to draw from parent node
    x1 = xPad + coord[i].first.x1() * XRatio;
    x2 = xPad + coord[i].first.x2() * XRatio;
    y1 = height() - (yPad + coord[i].first.y1() * YRatio);
    y2 = height() - (yPad + coord[i].first.y2() * YRatio);

    // change color if mouse points to current node
    if ((((m_mousePos.x() <= x1 && m_mousePos.x() >= x2) ||
	  (m_mousePos.x() >= x1 && m_mousePos.x() <= x2)) &&
	 (m_mousePos.y() > y1 && m_mousePos.y() <= y2)) || 
	selectedNodes.contains(QPoint(x1, y1))) {
      color = refColor;
      selectedNodes.push_back(QPoint(x2, y2));

      if (nodeId != -1)
	selectedLabels.push_back(nodeId);
    }

    // drawing from previous node
    font.setPixelSize(pixelSize);
    painter.setFont(font);
    painter.setPen(QPen(QBrush(color), lineWidth));
    painter.drawLine(x1, y1, x2, y1);
    painter.drawLine(x2, y1, x2, y2);

    // draw leaf label
    if (nodeId != -1) {
      font.setPixelSize(labelPixelSize);
      painter.setFont(font);
      QString nodeLabel = QString::number(nodeId);
      QFontMetrics fm(font);
      int width = fm.width(nodeLabel);
      painter.drawText(x2 - width / 2, y2 + pixelSize, nodeLabel);
    }

    // possibly point out selection medoid
    if (nodeId == m_iMed)
      painter.drawLine(x2, y2 + pixelSize + 5, x2, y2 + pixelSize * 3 + 5);
  }

  // drawing optimal cut
  painter.setPen(QPen(QBrush(Qt::black), lineWidth / 2, Qt::DotLine));
  y1 = height() - (yPad + m_dendogram->getBestCut() * YRatio);
  painter.drawLine(0, y1, width(), y1);
  
  // draw top-10 interlocutors
  if (!selectedLabels.isEmpty()) {
    
    int n(10);

    if (selectedLabels != m_selectedLabels) {

      // centroid of current selection
      arma::vec C;
      C.zeros(m_instances.first().n_rows);
    
      for (int i(0); i < selectedLabels.size(); i++)
	C += m_instances[selectedLabels[i]];
    
      C /= max(C);

      // retrieve mean social environment
      m_meanSocialEnv = getSocialEnv(C);

      // medoid of current selection
      m_iMed = m_dendogram->retrieveSubsetMedoid(selectedLabels);

      // retrieve median social environment
      m_medSocialEnv = getSocialEnv(m_instances[m_iMed] / max(m_instances[m_iMed]));

      m_selectedLabels = selectedLabels;
    }
    
    // display mean social environment
    font.setPixelSize(spkPixelSize);
    painter.setFont(font);

    int maxWidth(0);
    for (int i(0); i < n; i++) {

      QString fSpeaker = m_meanSocialEnv[i].second.first;
      QString sSpeaker = m_meanSocialEnv[i].second.second;

      QString interloc;

      if (fSpeaker == m_speaker)
	interloc = sSpeaker;
      else
	interloc = fSpeaker;
      
      QFontMetrics fm(font);
      int width = fm.width(interloc);

      if (width > maxWidth)
	maxWidth = width;
    }

    for (int i(0); i < n; i++) {
      
      qreal weight = m_meanSocialEnv[i].first;
      QString fSpeaker = m_meanSocialEnv[i].second.first;
      QString sSpeaker = m_meanSocialEnv[i].second.second;

      QString interloc;

      if (fSpeaker == m_speaker)
	interloc = sSpeaker;
      else
	interloc = fSpeaker;
      
      painter.setPen(getRGBColor(refColor, weight));
      painter.drawText(m_mousePos.x() - maxWidth, m_mousePos.y() + i * (spkPixelSize + 2), interloc);
    }
  }
  else
    m_iMed = -2;
}

QList<QPair<qreal, QPair<QString, QString> > > DendogramWidget::getSocialEnv(const arma::vec &U)
{
  QList<QPair<qreal, QPair<QString, QString> > > socialEnv;

  for (arma::uword i(0); i < U.n_rows; i++) {
    socialEnv.push_back(QPair<qreal, QPair<QString, QString> >(U(i), m_vecComponents[i]));
  }

  std::sort(socialEnv.begin(), socialEnv.end());
  std::reverse(socialEnv.begin(), socialEnv.end());

  return socialEnv;
}

void DendogramWidget::mouseMoveEvent(QMouseEvent *event)
{
  m_mousePos.setX(event->x());
  m_mousePos.setY(event->y());
  update();
}

QSize DendogramWidget::minimumSizeHint() const
{
  return QSize(600, 400);
}

QSize DendogramWidget::sizeHint() const
{
  return QSize(1200, 800);
}

QColor DendogramWidget::getRGBColor(const QColor &refColor, qreal weight)
{
  QColor color;
  qreal h, s, v, hsvA;

  // retrieving hue, saturation and value components of reference color
  refColor.getHsvF(&h, &s, &v, &hsvA);

  // computing saturation and value components for current vertex/edge
  // as a proportion of reference components
  s *= weight;
  // v *= weight;
  v = 1.0 - (1.0 - v) * weight;

  // color to be returned
  color = QColor::fromHsvF(h, s, v, hsvA);

  return color;
}
