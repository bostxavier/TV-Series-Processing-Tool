#include <QPainter>
#include <QDebug>

#include "SpeakersMonitor.h"

SpeakersMonitor::SpeakersMonitor(int labelHeight, QWidget *parent)
  : QGLWidget(parent),
    m_labelHeight(labelHeight)
{
  setFixedSize(200, 200);
}

///////////
// slots //
///////////

void SpeakersMonitor::speakersRetrieved(const igraph_t &graph)
{
  int nVertices = igraph_vcount(&graph);
  igraph_matrix_t res;
  igraph_matrix_init(&res, nVertices, 3);
  igraph_layout_sphere(&graph, &res);

  for (int i(0); i < igraph_matrix_nrow(&res); i++) {
    for (int j(0); j < igraph_matrix_ncol(&res); j++)
      qDebug() << MATRIX(res, i, j);
    qDebug();
  }
}

void SpeakersMonitor::positionChanged(qint64 position)
{
  m_position = position / 40 - 1;
  update();
}

void SpeakersMonitor::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
}
