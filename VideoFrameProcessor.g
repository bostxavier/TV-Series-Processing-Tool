#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <QObject>
#include <opencv2/core/core.hpp>

class FrameProcessor: public QObject
{
  Q_OBJECT

 public:
  enum HistoType {
    Lum, Hs, Hsv
  };
  FrameProcessor(int metrics = 0, QObject *parent = 0);

  public slots:
  void genVHisto(const cv::Mat &frame, int vBins);
  void genHsHisto(const cv::Mat &frame, int hBins, int sBins);
  void genHsvHisto(const cv::Mat &frame, int hBins, int sBins, int vBins);
  double distanceFromPrev(const cv::Mat &hist, HistoType histoType);
  void activL1();
  void activL2();
  void activCorrel();
  void activChiSqr();
  void activIntersect();
  void activHellinger();

  signals:
  void vHistoReady(const cv::Mat &vHisto);
  void hsHistoReady(const cv::Mat &hsHisto);
  void hsvHistoReady(const cv::Mat &hsvHisto);
  void diffVReady(qreal diff);
  void diffHsReady(qreal diff);
  void diffHsvReady(qreal diff);

 private:
  cv::Mat m_hsvPrev;
  cv::Mat m_hsPrev;
  cv::Mat m_vPrev;
  qreal m_prevHsvDist;
  qreal m_prevHsDist;
  qreal m_prevVDist;
  qreal m_currHsvDist;
  qreal m_currHsDist;
  qreal m_currVDist;
  int m_metrics;
  qreal m_normFactor;
};

#endif

