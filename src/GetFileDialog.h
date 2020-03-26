#ifndef GETFILEDIALOG_H
#define GETFILEDIALOG_H

#include <QDialog>
#include <QLineEdit>

class GetFileDialog: public QDialog
{
  Q_OBJECT

    public:
  GetFileDialog(const QString &openFileMsg, const QString &fileType, QWidget *parent = 0);
  QString getFName() const;

  public slots:
  void setFName();
  void checkForm();

  private:
  QString m_fName;
  QLineEdit *m_fNameLE;
  QString m_openFileMsg;
  QString  m_fileType;
};

#endif
