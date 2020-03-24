#include <QTime>
#include <QStyle>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "PlayerControls.h"

/////////////////
// constructor //
/////////////////

PlayerControls::PlayerControls(bool timeFmt, QWidget *parent)
  : QWidget(parent),
    m_timeFmt(timeFmt),
    playerState(QMediaPlayer::StoppedState),
    m_isMuted(false)
{
  m_durationSlider = new QSlider(Qt::Horizontal);
  connect(m_durationSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
  connect(m_durationSlider, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)));

  m_currentTimeLabel = new QLabel("--:--");
  QLabel *separatorLabel = new QLabel("/");
  m_durationLabel = new QLabel("--:--");

  QToolButton *previousButton = new QToolButton(this);
  previousButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));

  m_playButton = new QToolButton;
  m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  connect(m_playButton, SIGNAL(clicked()), this, SLOT(playClicked()));

  QToolButton *nextButton = new QToolButton(this);
  nextButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));

  QToolButton *stopButton = new QToolButton;
  stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
  connect(stopButton, SIGNAL(clicked()), this, SLOT(stopClicked()));

  m_muteButton = new QToolButton;
  m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
  connect(m_muteButton, SIGNAL(clicked()), this, SLOT(switchMuted()));

  QLabel *rateMult = new QLabel("x");
  m_rateBox = new QComboBox;
  m_rateBox->addItem("0.25");
  m_rateBox->addItem("0.5");
  m_rateBox->addItem("1.0");
  m_rateBox->addItem("2.0");
  m_rateBox->addItem("4.0");
  m_rateBox->setCurrentIndex(2);
  connect(m_rateBox, SIGNAL(currentTextChanged(const QString&)), this, SLOT(rateChanged(const QString&)));

  QHBoxLayout *durationLayout = new QHBoxLayout;
  durationLayout->addWidget(m_durationSlider);
  durationLayout->addWidget(m_currentTimeLabel);
  durationLayout->addWidget(separatorLabel);
  durationLayout->addWidget(m_durationLabel);

  QHBoxLayout *buttonBarLayout = new QHBoxLayout;
  buttonBarLayout->addWidget(previousButton);
  buttonBarLayout->addWidget(m_playButton);
  buttonBarLayout->addWidget(nextButton);
  buttonBarLayout->addWidget(stopButton);
  buttonBarLayout->addWidget(m_muteButton);
  buttonBarLayout->addWidget(m_durationSlider);
  buttonBarLayout->addWidget(m_durationLabel);
  buttonBarLayout->addWidget(m_rateBox);
  buttonBarLayout->addWidget(rateMult);

  QVBoxLayout *controlsLayout = new QVBoxLayout;
  controlsLayout->addLayout(durationLayout);
  controlsLayout->addLayout(buttonBarLayout);
    
  setLayout(controlsLayout);
}

////////////////
// destructor //
////////////////

PlayerControls::~PlayerControls()
{
}

void PlayerControls::reset()
{
  m_currentTimeLabel->setText("--:--");
  m_durationLabel->setText("--:--");
  setEnabled(false);
}

/////////////////////////////////////////
// public slots in reaction to signals //
//        emitted by the player        //
/////////////////////////////////////////

void PlayerControls::durationChanged(qint64 duration)
{
  QString tStr;

  m_durationSlider->setMaximum(duration);

  if (m_timeFmt) {
    duration /= 1000;
    QTime totalTime(duration / 3600, (duration / 60) % 60, duration % 60);

    if (duration > 3600)
      m_timeFormat = "hh:mm:ss";
    else
      m_timeFormat = "mm:ss";

    tStr = totalTime.toString(m_timeFormat);
  }
  else {
    QString durStr = QString::number(duration);
    QString paddedZero = QString("%1").arg(0, durStr.length(), 10, QChar('0'));
    m_currentTimeLabel->setText(paddedZero);
    tStr = QString::number(duration);
  }

  m_durationLabel->setText(tStr);
}

void PlayerControls::positionChanged(qint64 position)
{
  QString tStr;
  
  m_durationSlider->setSliderPosition(position);

  if (m_timeFmt) {
    position /= 1000;
    QTime currentTime((position / 3600) % 60, (position / 60) % 60, position % 60);
    tStr = currentTime.toString(m_timeFormat);
  }
  else {
    QString durStr = m_currentTimeLabel->text();
    QString paddedPos = QString("%1").arg(position, durStr.length(), 10, QChar('0'));
    tStr = paddedPos;
  }

  m_currentTimeLabel->setText(tStr);
}

void PlayerControls::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
  if (status == QMediaPlayer::EndOfMedia)
    stopClicked();
}

////////////////////////////////
// private slots used to emit //
//    signals to the player   //
////////////////////////////////

void PlayerControls::playClicked()
{
  if (playerState == QMediaPlayer::StoppedState || playerState == QMediaPlayer::PausedState) {
    emit play();
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    playerState = QMediaPlayer::PlayingState;
  }
  else {
    emit pause();
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playerState = QMediaPlayer::PausedState;
  }
}

void PlayerControls::stopClicked()
{
  m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  playerState = QMediaPlayer::StoppedState;
  emit stop();

  sliderMoved(0);
  
  m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
  m_muteButton->setEnabled(true);
  emit setMuted(false);

  m_rateBox->setCurrentIndex(2);
  emit playbackRateChanged(1.0);    
}

void PlayerControls::switchMuted()
{
  m_isMuted = !m_isMuted;

  emit setMuted(m_isMuted);

  if (m_isMuted)
    m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
  else
    m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
}

void PlayerControls::sliderValueChanged(int value)
{
  positionChanged(value);
  emit newPosition(value);
}

void PlayerControls::sliderMoved(int value)
{
  positionChanged(value);
  emit positionManuallyChanged(value);
}

void PlayerControls::rateChanged(const QString &rate)
{
  qreal doubleRate = rate.toDouble();
  emit playbackRateChanged(doubleRate);

  if (doubleRate != 1.0) {
    emit setMuted(true);
    m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    m_muteButton->setEnabled(false);
  }
  else {
    emit setMuted(false);
    m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    m_muteButton->setEnabled(true);
  }
}
