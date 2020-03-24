#include <QPainter>
#include <opencv2/imgproc/imgproc.hpp>

#include "DiffHistoGraph.h"

DiffHistoGraph::DiffHistoGraph(QColor color, bool minusY, int scale, int width, int height, QWidget *parent)
  : QWidget(parent),
    m_minusY(minusY),
    m_color(color)
{
  setFixedSize(width, height);
  m_scale = scale / 8.0;

  if (minusY)
    m_height = height / 2;
  else
    m_height = 0;
}

///////////
// slots //
///////////

void DiffHistoGraph::setScale(int scale)
{
  m_scale = scale / 8.0;
}

void DiffHistoGraph::appendDist(qreal dist)
{
  m_distList.append(dist);

  if (m_distList.size() > width())
    m_distList.removeFirst();

  update();
}

void DiffHistoGraph::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
  QPainter painter(this);

  // fill monitors background and set pen
  painter.fillRect(0, 0, width(), height(), QBrush(Qt::white));
  
  if (m_minusY) {
    painter.setPen(Qt::gray);
    painter.drawLine(0, m_height, width(), m_height);
  }

  int listSize = m_distList.size();

  painter.setPen(m_color);

  for (int i = 1; i < listSize; i++)
    painter.drawLine(i-1, height() * (1 - m_scale * m_distList[i-1]) - m_height, i, height() * (1 - m_scale * m_distList[i]) - m_height);
}
