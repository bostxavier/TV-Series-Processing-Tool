#include <QPainter>
#include <QDebug>

#include "GraphicsView.h"

/////////////////
// constructor //
/////////////////

GraphicsView::GraphicsView(QWidget *parent)
  : QGraphicsView(parent)
{
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

////////////////
// destructor //
////////////////

GraphicsView::~GraphicsView()
{
}

/////////////////////////////
// reimplemented functions //
/////////////////////////////

void GraphicsView::paintEvent(QPaintEvent *event)
{
  QGraphicsView::paintEvent(event);
  
  QPainter painter(this->viewport());
  painter.setPen(QPen(Qt::white, 2));

  int shift(18);
  
  for (int i(0); i < m_faces.size(); i++) {
    QRect bbox = m_faces[i].getBbox();
    painter.drawRect(bbox);
    QList<QPoint> landmarks = m_faces[i].getLandmarks();
    for (int i(0); i < landmarks.size(); i++)
      painter.drawEllipse(landmarks[i], 1, 1);
    QString id = QString::number(m_faces[i].getId());
    QString name = m_faces[i].getName();

    QRect boundingRect;
    painter.drawText(bbox.adjusted(0, -shift, 0, -shift), Qt::AlignHCenter|Qt::AlignTop, id, &boundingRect);
    painter.drawText(boundingRect, Qt::AlignHCenter|Qt::AlignTop, id, &boundingRect);

    if (name != "" ) {
      QStringList names = name.split(" ");
      painter.drawText(bbox.adjusted(0, shift, 0, shift), Qt::AlignHCenter|Qt::AlignBottom, names[0], &boundingRect);
      painter.drawText(boundingRect, Qt::AlignHCenter|Qt::AlignBottom, names[0], &boundingRect);
    }
  }
}

///////////
// slots //
///////////

void GraphicsView::displayFaces(const QList<Face> &faces)
{
  m_faces = faces;
  update();
}

