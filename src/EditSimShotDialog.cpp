#include <QGridLayout>
#include <QPixmap>
#include <QLabel>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "EditSimShotDialog.h"
#include "Convert.h"

#include <QDebug>

using namespace cv;

EditSimShotDialog::EditSimShotDialog(const QString &fName, int currIdx, QList<Shot *> shots, int nVignettes, int frameWidth, QWidget *parent)
  : QDialog(parent),
    m_nVignettes(nVignettes),
    m_refIdx(currIdx),
    m_currIdx(currIdx),
    m_shots(shots),
    m_nCamera(0)
{
  int fWidth;
  int fHeight;

  // vector containing the camera ids
  m_shotCamera.resize(m_shots.size());

  // retrieving movie features
  m_cap.release();
  m_cap.open(fName.toStdString());
  fWidth = m_cap.get(CV_CAP_PROP_FRAME_WIDTH);
  fHeight = m_cap.get(CV_CAP_PROP_FRAME_HEIGHT);
  m_vignetteWidth = frameWidth / m_nVignettes;
  m_vignetteHeight = m_vignetteWidth * fHeight / fWidth;

  // doesn't work anymore after updating Ubuntu to 14.04
  // m_frameDur = 1 / m_cap.get(CV_CAP_PROP_FPS) * 1000;
  m_frameDur = 1 / 25.0 * 1000;

  // initializing vignette list
  int i(m_refIdx);

  while (i >= m_refIdx - m_nVignettes + 1) {
    if (i < 0)
      m_vignettePositions.push_front(-1);
    else
      m_vignettePositions.push_front(m_shots[i]->getPosition() - m_frameDur);
    i--;
  }

  m_currentVignette = new QLabel;
  m_currentVignette->setPixmap(refShot());

  m_vignetteWidget = new VignetteWidget(m_nVignettes, frameWidth);
  m_vignetteWidget->setVideoCapture(fName);
  m_vignetteWidget->updateVignette(m_vignettePositions);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(m_currentVignette, 0, 0, Qt::AlignRight);
  layout->addWidget(m_vignetteWidget, 1, 0, Qt::AlignRight);

  setLayout(layout);
  
  setWindowTitle("Annotate similar shots - Shot " + QString::number(m_refIdx + 1));

  // setting deletion operations vector
  m_delOp.resize(m_shots.size());
  m_delOp.fill(false);
}

///////////////////////////////////
// slot called when pressing key //
///////////////////////////////////

void EditSimShotDialog::keyPressEvent(QKeyEvent *event)
{
  int toAddIdx; // index of shot to add to visible ones

  /********************************/
  /* going through previous shots */
  /********************************/

  if (event->key() == Qt::Key_Left && m_currIdx > m_nVignettes / 2) {

    m_currIdx--;
    toAddIdx = m_currIdx - m_nVignettes + 1;

    if (toAddIdx < 0)
      m_vignettePositions.push_front(-1);
    else
      m_vignettePositions.push_front(m_shots[toAddIdx]->getPosition() - m_frameDur);
    m_vignettePositions.pop_back();
  }
  
  else if (event->key() == Qt::Key_Right && m_currIdx < m_refIdx) {

    m_vignettePositions.push_back(m_shots[++m_currIdx]->getPosition() - m_frameDur);
    m_vignettePositions.pop_front();
  }

  /*******************************************************/
  /* linking current shot to previous one or ignoring it */
  /*******************************************************/

  else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Backspace) {

    // no similar shot found: increment camera label
    if (event->key() == Qt::Key_Backspace) {

      m_shots[m_refIdx]->setCamera(m_nCamera, Segment::Manual);
      m_shotCamera[m_refIdx] = m_nCamera++;

      // setting corresponding deletion operation to true
      m_delOp[m_refIdx] = true;
    }

    // similar shot found: label current shot with selected one
    else if (event->key() == Qt::Key_Return) {
      
      // linking the shots
      m_shots[m_refIdx]->setCamera(m_shotCamera[m_currIdx - 1], Segment::Manual);
      m_shotCamera[m_refIdx] = m_shotCamera[m_currIdx - 1];

      // setting corresponding deletion operation to false
      m_delOp[m_refIdx] = false;
    }

    // move to next shot position
    m_currIdx = ++m_refIdx;

    // last shot annotated: exit dialog
    if (m_refIdx == m_shots.size())
      accept();

    else {

      // re-initializing vignette list
      int i(m_refIdx);
      m_vignettePositions.clear();

      while (i >= m_refIdx - m_nVignettes + 1) {
	if (i < 0)
	  m_vignettePositions.push_front(-1);
	else
	  m_vignettePositions.push_front(m_shots[i]->getPosition() - m_frameDur);
	i--;
      }

      // set pixmap of current shot and window title
      m_currentVignette->setPixmap(refShot());
      setWindowTitle("Annotate similar shots - Shot " + QString::number(m_refIdx + 1));
      
      update();
    }
  }

  /*****************************/
  /* cancelling last operation */
  /*****************************/

  else if (event->key() == Qt::Key_Escape && m_refIdx > 0) {

    // updating indices
    m_currIdx = --m_refIdx;

    // last operation consisted in ignoring new shot
    if (m_delOp[m_refIdx])
      m_nCamera--;

    // last operation consisted in linking the shot to a previous one
    else
      m_shots[m_refIdx]->setCamera(-1, Segment::Manual);

    // updating vignette position list
    toAddIdx = m_currIdx - m_nVignettes + 1;

    if (toAddIdx < 0)
      m_vignettePositions.push_front(-1);
    else
      m_vignettePositions.push_front(m_shots[toAddIdx]->getPosition() - m_frameDur);

    m_vignettePositions.pop_back();

    // set pixmap of current shot and window title
    m_currentVignette->setPixmap(refShot());
    setWindowTitle("Annotate similar shots - Shot " + QString::number(m_refIdx + 1));

    update();
  }

  // updating corresponding images
  m_vignetteWidget->updateVignette(m_vignettePositions);
  m_vignetteWidget->update();
}

///////////////////////
// auxiliary methods //
///////////////////////

QPixmap EditSimShotDialog::refShot()
{
  Mat frame;

  // retrieving selected frame
  m_cap.set(CV_CAP_PROP_POS_MSEC, m_shots[m_refIdx]->getPosition());
  m_cap >> frame;
  cv::resize(frame, frame, Size(m_vignetteWidth, m_vignetteHeight));

  return QPixmap::fromImage(Convert::fromBGRMatToQImage(frame));
}
