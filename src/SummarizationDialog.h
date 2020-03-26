#ifndef SUMMARIZATIONDIALOG_H
#define SUMMARIZATIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>

class SummarizationDialog: public QDialog
{
  Q_OBJECT

 public:
  enum Method {
    Content, Style, Both, Random
  };

  SummarizationDialog(const QString &title, const QStringList &seasons, const QStringList &allSpeakers, const QList<QStringList> &seasonSpeakers, QWidget *parent = 0);

  int getSeasonNb() const;
  QString getSpeaker() const;
  int getDur() const;
  qreal getGranu() const;
  Method getMethod() const;

  public slots:
    void seasonChanged(int index);
    void activContent();
    void activStyle();
    void activBoth();
    void activRandom();

 signals:
    
 private:
    QStringList m_allSpeakers;
    QList<QStringList> m_seasonSpeakers;
    QComboBox *m_seasonCB;
    QComboBox *m_charCB;
    QSpinBox *m_durSB;
    QSpinBox *m_granuSB;
    SummarizationDialog::Method m_method;
};

#endif
