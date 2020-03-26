#ifndef HISTOMONITOR_H
#define HISTOMONITOR_H

#include <QWidget>
#include <opencv2/core/core.hpp>
#include "VideoFrameProcessor.h"
#include "VHistoWidget.h"
#include "HsHistoWidget.h"
#include "DiffHistoGraph.h"

class HistoMonitor: public QWidget
{
  Q_OBJECT
  
 public:
  HistoMonitor(QWidget *parent = 0, int hBins = 32, int sBins = 32, int vBins = 128);

 signals:
  void distFromPrev(qreal distance);

  public slots:
   void processMat(const cv::Mat &bgrMat);
   void setHBins(int hBins);
   void setSBins(int sBins);
   void setVBins(int vBins);

 private:
   VideoFrameProcessor *m_processor;
   VHistoWidget *m_vHisto;
   HsHistoWidget *m_hsHisto;
   cv::Mat m_prevHsvHisto;
   int m_hBins;
   int m_sBins;
   int m_vBins;
};

#endif
