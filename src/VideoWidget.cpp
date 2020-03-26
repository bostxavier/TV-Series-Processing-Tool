#include <QPainter>
#include <QDebug>

#include "VideoWidget.h"

/////////////////
// constructor //
/////////////////

VideoWidget::VideoWidget(QWidget *parent): QVideoWidget(parent)
{
}

////////////////
// destructor //
////////////////

VideoWidget::~VideoWidget()
{
}

/////////////////////////////
// reimplemented functions //
/////////////////////////////

void VideoWidget::paintEvent(QPaintEvent *event)
{
  qDebug() << "enters paintEvent()";
  QVideoWidget::paintEvent(event);

  QPainter painter(this);
  painter.setPen(Qt::white);

  painter.drawLine(0, 0, 480, 240);
}
