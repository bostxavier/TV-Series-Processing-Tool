#ifndef CONVERT_H
#define CONVERT_H

#include <QImage>
#include <QVideoFrame>

#include <opencv2/core/core.hpp>

class Convert
{
 public:
  static QImage fromYuvToRgb(QVideoFrame frame);
  static QImage fromYuvToGray(QVideoFrame frame);
  static cv::Mat fromYuvToBgrMat(QVideoFrame frame);
  static QImage fromBGRMatToQImage(const cv::Mat &frame);
};

#endif
