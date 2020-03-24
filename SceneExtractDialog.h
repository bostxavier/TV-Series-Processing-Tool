#ifndef SCENEEXTRACTDIALOG_H
#define SCENEEXTRACTDIALOG_H

#include <QDialog>

#include "Segment.h"

class SceneExtractDialog: public QDialog
{
  Q_OBJECT

 public:
  enum VisualSrc {
    Man, Auto
  };

  SceneExtractDialog(QWidget *parent = 0);
  Segment::Source getVisualSource() const;

  public slots:
    void activAutoVSrc();
    void activManVSrc();

    private:
    Segment::Source m_vSrc;
};

#endif


