#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <QPainter>
#include "HsHistoWidget.h"

using namespace cv;
using namespace std;

HsHistoWidget::HsHistoWidget(int width, int height, int paletteHeight, QWidget *parent)
  : QWidget(parent),
    m_width(width),
    m_height(height),
    m_paletteHeight(paletteHeight)
{
  setFixedSize(m_width, m_height + m_paletteHeight + 1);
}

///////////
// slots //
///////////

void HsHistoWidget::setHisto(const Mat &hsHisto)
{
  // normalizing histogram between 0 and 255 for drawing purpose
  normalize(hsHisto, m_hsHisto, 0, 255, NORM_MINMAX, -1, Mat());
  update();
}

void HsHistoWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
  float binWidth = static_cast<float>(m_width) / m_hsHisto.rows;
  float binHeight = static_cast<float>(m_height) / m_hsHisto.cols;

  QPainter painter(this);
  float hNormFac = 360.0 / m_hsHisto.rows;

  painter.fillRect(0, 0, m_width, m_height, QBrush(Qt::white));

  for (int i = 1; i <= m_hsHisto.rows; i++)  {
    
    // drawing hue palette
    QRectF palette(QPointF((i-1) * binWidth, m_height + 1),
		   QPointF(i * binWidth, m_height + 1 + m_paletteHeight));
    painter.fillRect(palette, QColor::fromHsv(qRound((i-1) * hNormFac), 255, 255));

    // plotting histogram values
    for (int j = 1; j <= m_hsHisto.cols; j++) {
      QRectF rectangle(QPointF((i-1) * binWidth, m_height - (j-1) * binHeight),
		       QPointF(i * binWidth, m_height - j * binHeight));
      uchar grayLevel = qRound(m_hsHisto.at<float>(i, j));
      QColor grayColor(grayLevel, grayLevel, grayLevel);
      painter.fillRect(rectangle, QBrush(grayColor));
    }
  }
}
