#include <QAbstractVideoBuffer>
#include <QDebug>

#include <opencv2/imgproc/imgproc.hpp>

#include "Convert.h"

using namespace cv;

QImage Convert::fromYuvToRgb(QVideoFrame frame)
{
  int y, u, v;
  int c, d, e;
  int r, g, b;
  int height = frame.height();
  int width = frame.width();
  int size = height * width;
  QImage image(width, height, QImage::Format_RGB888);

  frame.map(QAbstractVideoBuffer::ReadOnly);
  uchar *ptr = frame.bits();

  for (int i(0); i < height; i++) {
    for (int j(0); j < width; j++) {

      // retrieve Y, U, V components
      y = *(ptr + i * width + j);
      u = *(ptr + size + i / 2 * width / 2 + j / 2);
      v = *(ptr + size + size / 4 + i / 2 * width / 2 + j / 2);

      // compute R, G, B components
      c = y - 16;
      d = u - 128;
      e = v - 128;
      r = (298*c + 409*e + 128) >> 8;
      g = (298*c - 100*d - 208*e + 128) >> 8;
      b = (298*c + 516*d + 128) >> 8;

      // clamp R, G, B components
      r = (r > 255) ? 255 : r;
      g = (g > 255) ? 255 : g;
      b = (b > 255) ? 255 : b;
      r = (r < 0) ? 0 : r;
      g = (g < 0) ? 0 : g;
      b = (b < 0) ? 0 : b;

      image.setPixel(j, i, qRgb(r, g, b));
    }
  }

  frame.unmap();
  return image;
}

Mat Convert::fromYuvToBgrMat(QVideoFrame frame)
{
  QImage qimage = Convert::fromYuvToRgb(frame).rgbSwapped();
  Mat image(frame.height(), frame.width(), CV_8UC3, qimage.bits());
  
  return image.clone();
}

QImage Convert::fromYuvToGray(QVideoFrame frame)
{
  int y;
  int height = frame.height();
  int width = frame.width();
  int size = height * width;
  QImage image(width, height, QImage::Format_Indexed8);

  frame.map(QAbstractVideoBuffer::ReadOnly);
  uchar *fPtr = frame.bits();                // pointer to video frame pixels
  uchar *iPtr = image.bits();                // pointer to image pixels

  for (int i(0); i < size; i++) {

    // retrieve Y component
    y = *fPtr;

    // set image pixel
    *iPtr = y;

    // increment frame and image pointers
    fPtr++;
    iPtr++;
  }

  frame.unmap();
  return image;
}

QImage Convert::fromBGRMatToQImage(const Mat &frame)
{
  Mat tmp;
  cvtColor(frame, tmp, CV_BGR2RGB);
  QImage dst1((uchar *) tmp.data, tmp.cols, tmp.rows, tmp.step, QImage::Format_RGB888);
  QImage dst2(dst1);
  dst2.detach();

  return dst2;
}
