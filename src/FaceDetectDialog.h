#ifndef FACEDETECTDIALOG_H
#define FACEDETECTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QSpinBox>

class FaceDetectDialog: public QDialog
{
  Q_OBJECT

 public:
  
  enum Method {
    OpenCV, Zhu, ExtData
  };

  FaceDetectDialog(const QString &title, QWidget *parent = 0);

public slots:
  void activOpenCV();
  void activZhu();
  void activExtData();

Method getMethod() const;
int getMinHeight() const;

private:
Method m_method;
QLabel *m_minHeightLabel;
QSpinBox *m_minHeightSB;
};

#endif
