#ifndef COMPAREIMAGEDIALOG_H
#define COMPAREIMAGEDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QLabel>

class CompareImageDialog: public QDialog
{
  Q_OBJECT

    public:
  CompareImageDialog(const QString &title, int V, int H, int S, int defValue1, int defValue2, const QString &threshLab1, const QString &threshLab2 = QString(), QWidget *parent = 0);
  
  int getHistoType();
  int getMetrics();
  int getThreshold1();
  int getThreshold2();
  int getNVBlock();
  int getNHBlock();
  int getNVBins();
  int getNHBins();
  int getNSBins();

  public slots:
  void activLHisto();
  void activHsHisto();
  void activHsvHisto();
  void activL1();
  void activL2();
  void activCorrel();
  void activChiSqr();
  void activIntersect();
  void activHellinger();

  private:
  int m_histoType;
  int m_metrics;
  QSpinBox *m_thresholdSB1;
  QSpinBox *m_thresholdSB2;
  QSpinBox *m_nVBlockSB;
  QSpinBox *m_nHBlockSB;
  QLabel *m_nVBinsLabel;
  QLabel *m_nHBinsLabel;
  QLabel *m_nSBinsLabel;
  QSpinBox *m_nVBinsSB;
  QSpinBox *m_nHBinsSB;
  QSpinBox *m_nSBinsSB;
};

#endif
