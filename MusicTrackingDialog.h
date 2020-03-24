#ifndef MUSICTRACKINGDIALOG_H
#define MUSICTRACKINGDIALOG_H

#include <QDialog>
#include <QSpinBox>

class MusicTrackingDialog: public QDialog
{
  Q_OBJECT

 public:
  MusicTrackingDialog(const QString &title, QWidget *parent = 0);

  int getSampleRate() const;
  int getMtWindowSize() const;
  int getMtHopSize() const;
  int getChromaStaticFrameSize() const;
  int getChromaDynamicFrameSize() const;

  public slots:
    void adjustExtrema(int value);
    
    private:
  QSpinBox *m_sampleRateSB;
  QSpinBox *m_mtWindowSizeSB;
  QSpinBox *m_mtHopSizeSB;
  QSpinBox *m_chromaStaticFrameSizeSB;
  QSpinBox *m_chromaDynamicFrameSizeSB;
};

#endif
