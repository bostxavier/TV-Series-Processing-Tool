#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QVideoWidget>

class VideoWidget: public QVideoWidget
{
 public:
  VideoWidget(QWidget *parent = 0);
  ~VideoWidget();

 protected:
  void paintEvent(QPaintEvent *event);

 private:
};

#endif
