#include <opencv2/imgproc/imgproc.hpp>

#include "VideoFrameProcessor.h"

using namespace cv;

VideoFrameProcessor::VideoFrameProcessor(int metrics, QObject *parent)
  : QObject(parent),
    m_metrics(metrics),
    m_normFactor(1.0)
{
}

QVector<cv::Mat> VideoFrameProcessor::splitImage(const Mat &frame, int nVBlock, int nHBlock)
{
  QVector<Mat> blocks;
  int blockVSize(frame.rows / nVBlock);
  int blockHSize(frame.cols / nHBlock);
  
  for (int i(0); i < frame.rows; i += blockVSize)
    for (int j(0); j < frame.cols; j += blockHSize)
      if (i + blockVSize <= frame.rows && j + blockHSize <= frame.cols)
	blocks.push_back(frame(Rect(j, i, blockHSize, blockVSize)));

  return blocks;
}

///////////
// slots //
///////////

cv::Mat VideoFrameProcessor::genVHisto(const cv::Mat &frame, int vBins)
{
  // histogram
  Mat vHisto;

  // use the 3-rd channel
  int channels[] = { 2 };

  // value varies from 0 to 255
  float range[] = { 0, 256 };
  const float* histRange = { range };

  // computing and normalizing histogram
  calcHist(&frame, 1, channels, Mat(), vHisto, 1, &vBins, &histRange, true, false);
  normalize(vHisto, vHisto, 1, 0, NORM_L1);

  return vHisto;
}
 
cv::Mat VideoFrameProcessor::genHsHisto(const cv::Mat &frame, int hBins, int sBins)
{
  // histogram
  Mat hsHisto;

  // use the 1-st and 2-nd channels
  int channels[] = { 0, 1 };
  
  // histogram sizes as defined by user
  int histSize[] = { hBins, sBins };

  // hue varies from 0 to 179, saturation from 0 to 255
  float hRange[] = { 0, 180 };
  float sRange[] = { 0, 256 };
  const float* ranges[] = { hRange, sRange };

  // computing and normalizing histogram
  calcHist(&frame, 1, channels, Mat(), hsHisto, 2, histSize, ranges, true, false);
  normalize(hsHisto, hsHisto, 1, 0, NORM_L1);

  return hsHisto;
}
 
cv::Mat VideoFrameProcessor::genHsvHisto(const cv::Mat &frame, int hBins, int sBins, int vBins)
{
  // histogram
  Mat hsvHisto;

  // use the three channels
  int channels[] = { 0, 1, 2 };
  
  // histogram sizes as defined by user
  int histSize[] = { hBins, sBins, vBins };

  // hue ranges from 0 to 179, saturation from 0 to 255 and value from 0 to 255
  float hRange[] = { 0, 180 };
  float sRange[] = { 0, 256 };
  float vRange[] = { 0, 256 };
  const float* ranges[] = { hRange, sRange, vRange };

  // computing and normalizing histogram
  calcHist(&frame, 1, channels, Mat(), hsvHisto, 3, histSize, ranges, true, false);
  normalize(hsvHisto, hsvHisto, 1, 0, NORM_L1);

  return hsvHisto;
}

double VideoFrameProcessor::distanceFromPrev(const Mat &hist, const Mat &prevHist)
{
  double dist(1.0);
  
  if (hist.size() == prevHist.size()) {
    if (m_metrics == NORM_L1 || m_metrics == NORM_L2)
      dist = norm(hist, prevHist, m_metrics);
    else
      dist = compareHist(hist, prevHist, m_metrics);
  }
  else
    dist = -1.0;
  
  if (m_metrics == CV_COMP_CORREL)
    dist = 1.0 - dist;

  return dist;
}

void VideoFrameProcessor::activL1()
{
  m_metrics = NORM_L1;
  m_normFactor = 0.5;
}

void VideoFrameProcessor::activL2()
{
  m_metrics = NORM_L2;
  m_normFactor = 1 / sqrt(2);
}

void VideoFrameProcessor::activCorrel()
{
  m_metrics = CV_COMP_CORREL;
  m_normFactor = 1.0;
}
 
void VideoFrameProcessor::activChiSqr()
{
  m_metrics = CV_COMP_CHISQR;
  m_normFactor = 1.0;
}
 
void VideoFrameProcessor::activIntersect()
{
  m_metrics = CV_COMP_INTERSECT;
  m_normFactor = 1.0;
}
 
void VideoFrameProcessor::activHellinger()
{
  m_metrics = CV_COMP_HELLINGER;
  m_normFactor = 1.0;
}
