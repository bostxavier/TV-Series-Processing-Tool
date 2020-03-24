#ifndef PLAYERCONTROLS_H
#define PLAYERCONTROLS_H

#include <QWidget>
#include <QToolButton>
#include <QMediaPlayer>
#include <QLabel>
#include <QSlider>
#include <QComboBox>

class PlayerControls: public QWidget
{
  Q_OBJECT

    public:
  PlayerControls(bool timeFmt = true, QWidget *parent = 0);
  ~PlayerControls();
  void reset();
  
  public slots:
  void durationChanged(qint64 duration);
  void positionChanged(qint64 position);
  void mediaStatusChanged(QMediaPlayer::MediaStatus status);
  void playClicked();
  void stopClicked();

 signals:
  void play();
  void pause();
  void stop();
  void setMuted(bool muted);
  void positionManuallyChanged(qint64 value);
  void newPosition(qint64 value);
  void playbackRateChanged(qreal rate);

  private slots:
  void switchMuted();
  void sliderValueChanged(int value);
  void sliderMoved(int value);
  void rateChanged(const QString &rate);

 private:
  bool m_timeFmt;
  QToolButton *m_playButton;
  QToolButton *m_muteButton;
  QMediaPlayer::State playerState;
  bool m_isMuted;
  QLabel *m_currentTimeLabel;
  QLabel *m_durationLabel;
  QSlider *m_durationSlider;
  QComboBox *m_rateBox;
  QString m_timeFormat;
};

#endif
