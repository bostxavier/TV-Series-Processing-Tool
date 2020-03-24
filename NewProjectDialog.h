#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QString>
#include <QLineEdit>
#include <QSpinBox>

class NewProjectDialog: public QDialog
{
  Q_OBJECT
 
 public:
  NewProjectDialog(QWidget *parent = 0, QString const &projectName = QString(), QString const &seriesName = QString());
  ~NewProjectDialog();
  QString getProjectName() const;
  QString getSeriesName() const;
  QString getEpName() const;
  QString getEpFName() const;
  int getSeasNbr() const;
  int getEpNbr() const;
    
  public slots:
    void setMovieFName();
  void checkForm();

 private:
  QString m_projectName;
  QString m_seriesName;
  QString m_epName;
  QString m_epFName;
  int m_seasNbr;
  int m_epNbr;
  QLineEdit *m_projectNameLE;
  QLineEdit *m_seriesNameLE;
  QSpinBox *m_seasNbrSB;
  QSpinBox *m_epNbrSB;
  QLineEdit *m_epFNameLE;
  QLineEdit *m_epNameLE;
};

#endif
