#ifndef VIGNETTEWIDGET_H
#define VIGNETTEWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QImage>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

class VignetteWidget: public QWidget
{
  Q_OBJECT

    public:
  VignetteWidget(int nVignettes, int frameWidth, QWidget *parent = 0);
  void setVideoCapture(const QString &fName);

 protected:
  void paintEvent(QPaintEvent *event);

  public slots:
    void updateVignette(QList<qint64> positionList);

  private:
  cv::VideoCapture m_cap;
  int m_width;
  int m_height;
  int m_shift;
  QList<QImage> m_vignettes;
  int m_nVignettes;
  qint64 m_currentPosition;
};

#endif
