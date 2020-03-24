#ifndef VIDEOFRAMEPROCESSOR_H
#define VIDEOFRAMEPROCESSOR_H

#include <QVector>

#include <QObject>
#include <opencv2/core/core.hpp>

class VideoFrameProcessor: public QObject
{
  Q_OBJECT

 public:
  enum HistoType {
    Lum, Hs, Hsv
  };
  VideoFrameProcessor(int metrics = 0, QObject *parent = 0);
  QVector<cv::Mat> splitImage(const cv::Mat &frame, int nVBlock, int nHBlock);

  public slots:
  cv::Mat genVHisto(const cv::Mat &frame, int vBins);
  cv::Mat genHsHisto(const cv::Mat &frame, int hBins, int sBins);
  cv::Mat genHsvHisto(const cv::Mat &frame, int hBins, int sBins, int vBins);
  double distanceFromPrev(const cv::Mat &hist, const cv::Mat &prevHist);
  void activL1();
  void activL2();
  void activCorrel();
  void activChiSqr();
  void activIntersect();
  void activHellinger();

 private:
  int m_metrics;
  qreal m_normFactor;
};

#endif

