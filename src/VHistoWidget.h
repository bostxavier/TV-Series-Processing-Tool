#ifndef VHISTOWIDGET_H
#define VHISTOWIGET_H

#include <QWidget>
#include <opencv2/core/core.hpp>
#include "VideoFrameProcessor.h"

class VHistoWidget: public QWidget
{
  Q_OBJECT

 public:
  VHistoWidget(int scale, int width = 210, int height = 130, int paletteHeight = 10, QWidget *parent = 0);
  void setHisto(const cv::Mat &vHisto);

  public slots:
    void setScale(int scale);
  
 protected:
    void paintEvent(QPaintEvent *event);

 private:
    int m_width;
    int m_height;
    int m_paletteHeight;
    int m_scale;
    VideoFrameProcessor *m_processor;
    cv::Mat m_vHisto;
};

#endif
