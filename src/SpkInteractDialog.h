#ifndef SPKINTERACTDIALOG_H
#define SPKINTERACTDIALOG_H

#include <QDialog>
#include <QSpinBox>

class SpkInteractDialog: public QDialog
{
  Q_OBJECT

 public:
  enum InteractType {
    Ref, CoOccurr, Sequential
  };

  enum RefUnit {
    Scene, LSU
  };

  SpkInteractDialog(const QString &title, QWidget *parent = 0);
  InteractType getInteractType() const;
  RefUnit getRefUnit() const;
  int getNbDiscard() const;
  int getInterThresh() const;

  public slots:
    void activCoOccurr();
    void activSequential();
    void activScene();
    void activLSU();

 private:
    InteractType m_type;
    RefUnit m_unit;
    QSpinBox *m_nbDiscardSB;
    QSpinBox *m_interThreshSB;
};

#endif
