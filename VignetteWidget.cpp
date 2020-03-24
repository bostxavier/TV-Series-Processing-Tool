#include <QPainter>
#include <QDebug>

#include <opencv2/imgproc/imgproc.hpp>

#include "VignetteWidget.h"
#include "Convert.h"

using namespace cv;

VignetteWidget::VignetteWidget(int nVignettes, int frameWidth, QWidget *parent)
  : QWidget(parent),
    m_width(frameWidth / nVignettes),
    m_height(0),
    m_shift(0),
    m_nVignettes(nVignettes),
    m_currentPosition(-2)
{
  setFixedSize(frameWidth, m_height);
}

void VignetteWidget::setVideoCapture(const QString &fName)
{
  m_cap.release();
  m_cap.open(fName.toStdString());
  int frameWidth = m_cap.get(CV_CAP_PROP_FRAME_WIDTH);
  int frameHeight = m_cap.get(CV_CAP_PROP_FRAME_HEIGHT);
  m_height = m_width * frameHeight / frameWidth;
  setFixedSize(width(), m_height);
}

///////////
// slots //
///////////

void VignetteWidget::updateVignette(QList<qint64> positionList)
{
  Mat frame(m_height, m_width, CV_8UC3, Scalar(0, 0, 0));
  int step = m_width / 10;

  qint64 position = positionList[m_nVignettes / 2];

  if (m_currentPosition == positionList[m_nVignettes / 2 - 1]) {

    if (positionList[m_nVignettes - 1] != -1) {
      m_cap.set(CV_CAP_PROP_POS_MSEC, positionList[m_nVignettes - 1]);
      m_cap >> frame;
      cv::resize(frame, frame, Size(m_width, m_height));
    }

    m_vignettes.push_back(Convert::fromBGRMatToQImage(frame));
    m_vignettes.pop_front();

    m_shift = m_width;
    while (m_shift > 0) {
      m_shift -= step;
    repaint();
    }
  }

  else if (m_currentPosition == positionList[m_nVignettes / 2 + 1]) {

    if (positionList[0] != -1) {
      m_cap.set(CV_CAP_PROP_POS_MSEC, positionList[0]);
      m_cap >> frame;
      cv::resize(frame, frame, Size(m_width, m_height));
    }

    m_vignettes.push_front(Convert::fromBGRMatToQImage(frame));
    m_vignettes.pop_back();

    m_shift = -m_width;
    while (m_shift < 0) {
      m_shift += step;
    repaint();
    }
  }

  else if (m_currentPosition != position) {

    m_vignettes.clear();

    for (int i(0); i < positionList.size(); i++) {

      if (positionList[i] != -1) {
	m_cap.set(CV_CAP_PROP_POS_MSEC, positionList[i]);
	m_cap >> frame;
	cv::resize(frame, frame, Size(m_width, m_height));
      }

      m_vignettes.push_back(Convert::fromBGRMatToQImage(frame));
    }

    m_shift = 0;
    repaint();
  }

  m_currentPosition = position;
}

void VignetteWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setPen(Qt::white);
  for (int i(0); i < m_vignettes.size(); i++)
    painter.drawImage(QRect(m_width * i + m_shift, 0, m_width, m_height), m_vignettes[i]);
  for (int i(1); i < m_vignettes.size(); i++)
    painter.drawLine(m_width * i + m_shift, 0, m_width * i + m_shift, m_height);
}
