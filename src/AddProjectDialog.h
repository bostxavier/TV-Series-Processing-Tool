#ifndef ADDPROJECTDIALOG_H
#define ADDPROJECTDIALOG_H

#include <QDialog>
#include <QString>
#include <QLineEdit>
#include <QSpinBox>

class AddProjectDialog: public QDialog
{
  Q_OBJECT
 
 public:
  AddProjectDialog(const QString &projectName, const QString &seriesName, QWidget *parent = 0);
  ~AddProjectDialog();
  QString getProjectName() const;
  QString getSecProjectFName() const;
    
  public slots:
    void setMovieFName();
  void checkForm();

 private:
  QString m_projectName;
  QString m_secProjectFName;
  QLineEdit *m_projectNameLE;
  QLineEdit *m_seriesNameLE;
  QLineEdit *m_secProjectFNameLE;
};

#endif
