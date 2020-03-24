#ifndef EDITSIMSHOTDIALOG_H
#define EDITSIMSHOTDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QPixmap>
#include <QLabel>

#include "VignetteWidget.h"
#include "Shot.h"

class EditSimShotDialog: public QDialog
{
  Q_OBJECT

 public:
  EditSimShotDialog(const QString &fName, int currIdx, QList<Shot *> shots, int nVignettes, int frameWidth, QWidget *parent = 0);
  
 protected:
  void keyPressEvent(QKeyEvent *event);

 signals:
  void labelSimShot(qint64 position, int nCamera, Segment::Source source); 

 private:
  QPixmap refShot();

  cv::VideoCapture m_cap;
  int m_vignetteWidth;
  int m_vignetteHeight;
  int m_frameDur;
  QLabel *m_currentVignette;
  int m_nVignettes;
  VignetteWidget *m_vignetteWidget;
  int m_refIdx;
  int m_currIdx;
  QList<Shot *> m_shots;
  QList<qint64> m_vignettePositions;
  QVector<int> m_shotCamera;
  QVector<bool> m_delOp;
  int m_nCamera;
};

#endif
