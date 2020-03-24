#include <QPainter>
#include "VHistoWidget.h"
#include <iostream>

using namespace cv;
using namespace std;

VHistoWidget::VHistoWidget(int scale, int width, int height, int paletteHeight, QWidget *parent)
  : QWidget(parent),
    m_width(width),
    m_height(height),
    m_paletteHeight(paletteHeight),
    m_scale(scale)
{
  setFixedSize(m_width, m_height + m_paletteHeight + 1);
}

///////////
// slots //
///////////

void VHistoWidget::setHisto(const Mat &vHisto)
{
  m_vHisto = vHisto;
  update();
}

void VHistoWidget::setScale(int scale)
{
  m_scale = scale;
  update();
}

void VHistoWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
  float binWidth = static_cast<float>(m_width) / m_vHisto.rows;

  QPainter painter(this);
  painter.fillRect(0, 0, m_width, m_height, QBrush(Qt::white));
  float vNormFac = 256.0 / m_vHisto.rows;

  for (int i = 1; i <= m_vHisto.rows; i++) {
    QRectF rectangle(QPointF((i-1) * binWidth,
			     m_height * (1 - m_scale * m_vHisto.at<float>(i-1))),
		     QPointF(i * binWidth,
			     m_height));
    painter.fillRect(rectangle, QBrush(Qt::black));

    QRectF palette(QPointF((i-1) * binWidth, m_height + 1),
		   QPointF(i * binWidth, m_height + 1 + m_paletteHeight));
    QColor vLevel(qRound((i-1) * vNormFac), qRound((i-1) * vNormFac), qRound((i-1) * vNormFac));
    painter.fillRect(palette, vLevel);
  }
}
