#ifndef HSHISTOWIDGET_H
#define HSHISTOWIDGET_H

#include <QWidget>
#include <opencv2/core/core.hpp>
#include "VideoFrameProcessor.h"

class HsHistoWidget: public QWidget
{
  Q_OBJECT

 public:
  HsHistoWidget(int width = 130, int height = 130, int paletteHeight = 10, QWidget *parent = 0);
  void setHisto(const cv::Mat &hsHisto);
  
 protected:
  void paintEvent(QPaintEvent *event);

 private:
  int m_width;
  int m_height;
  int m_paletteHeight;
  cv::Mat m_hsHisto;
};

#endif
