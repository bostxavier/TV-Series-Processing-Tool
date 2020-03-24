#include <QUrl>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSize>
#include <QMediaContent>
#include <QPainter>

#include "VideoPlayer.h"

#include "Convert.h"

/////////////////
// constructor //
/////////////////

VideoPlayer::VideoPlayer(int nVignettes, QWidget *parent)
  : QWidget(parent),
    m_currIndex(-1),
    m_segIdx(0),
    m_summIdx(-1),
    m_currText(QString()),
    m_currSpeechSegment(0)
{
  m_player = new QMediaPlayer(this);
  m_playlist = new QMediaPlaylist;
  m_movieMonitor = new MovieMonitor;
  m_vignetteWidget = new VignetteWidget(nVignettes, 480);
  m_view = new GraphicsView;
  m_scene = new QGraphicsScene;

  m_playlist->setPlaybackMode(QMediaPlaylist::Sequential);
  m_player->setPlaylist(m_playlist);

  const QPixmap logo("pictures/logo_lia.png");
  m_pixmapItem = new QGraphicsPixmapItem(logo);
  m_videoItem = new QGraphicsVideoItem;

  m_scene->addItem(m_pixmapItem);
  m_scene->addItem(m_videoItem);
  m_videoItem->hide();

  m_view->setScene(m_scene);
  m_view->setFixedSize(m_pixmapItem->pixmap().size());

  m_player->setVideoOutput(m_videoItem);

  m_subEditLabel = new QTextEdit;
  m_subEditLabel->setFixedSize(480, 60);
  m_subEditLabel->setReadOnly(true);
  m_subEditLabel->setAcceptRichText(false);

  QHBoxLayout *audioLayout = new QHBoxLayout;
  audioLayout->addWidget(m_subEditLabel);

  m_controls = new PlayerControls;

  m_probe = new QVideoProbe;
  m_probe->setSource(m_player);

  m_segments.push_back(QPair<qint64, qint64>(0, 5000000));

  QVBoxLayout *vLayout = new QVBoxLayout;
  vLayout->addWidget(m_vignetteWidget);
  vLayout->addWidget(m_view);
  vLayout->addLayout(audioLayout);
  vLayout->addWidget(m_controls);

  QHBoxLayout *layout = new QHBoxLayout;
  layout->addWidget(m_movieMonitor);
  layout->addLayout(vLayout);
  setLayout(layout);

  m_controls->setEnabled(false);

  connect(m_controls, SIGNAL(play()), m_player, SLOT(play()));
  connect(m_controls, SIGNAL(pause()), m_player, SLOT(pause()));
  connect(m_controls, SIGNAL(stop()), m_player, SLOT(stop()));
  connect(m_controls, SIGNAL(setMuted(bool)), m_player, SLOT(setMuted(bool)));
  connect(m_controls, SIGNAL(positionManuallyChanged(qint64)), this, SLOT(positionManuallyChanged(qint64)));
  connect(m_controls, SIGNAL(playbackRateChanged(qreal)), m_player, SLOT(setPlaybackRate(qreal)));

  connect(m_player, SIGNAL(durationChanged(qint64)), m_controls, SLOT(durationChanged(qint64)));
  connect(m_player, SIGNAL(durationChanged(qint64)), m_movieMonitor, SLOT(durationChanged(qint64)));
  connect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
  connect(m_player, SIGNAL(positionChanged(qint64)), m_controls, SLOT(positionChanged(qint64)));
  connect(m_player, SIGNAL(positionChanged(qint64)), m_movieMonitor, SLOT(positionChanged(qint64)));
  connect(m_player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(stateChanged(QMediaPlayer::State)));
  connect(m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), m_controls, SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));
  connect(m_probe, SIGNAL(videoFrameProbed(const QVideoFrame &)), m_movieMonitor, SLOT(processFrame(QVideoFrame)));
  connect(m_subEditLabel, SIGNAL(textChanged()), this, SLOT(subTextChanged()));

  connect(this, SIGNAL(showHisto(bool)), m_movieMonitor, SLOT(showHisto(bool)));
  connect(this, SIGNAL(viewSegmentation(bool, bool)), m_movieMonitor, SLOT(viewSegmentation(bool, bool)));
  connect(this, SIGNAL(showUtteranceTree(bool)), m_movieMonitor, SLOT(viewUtteranceTree(bool)));
  connect(this, SIGNAL(initDiarData(const arma::mat &, const arma::mat &, const arma::mat &, QList<SpeechSegment *>)), m_movieMonitor, SLOT(setDiarData(const arma::mat &, const arma::mat &, const arma::mat &, QList<SpeechSegment *>)));
  connect(this, SIGNAL(grabCurrentPattern(const QPair<int, int> &)), m_movieMonitor, SLOT(grabCurrentPattern(const QPair<int, int> &)));
  connect(this, SIGNAL(showSpeakers(bool)), m_movieMonitor, SLOT(viewSpeakers(bool)));
  connect(this, SIGNAL(grabShots(QList<Segment *>, Segment::Source)), m_movieMonitor, SLOT(getShots(QList<Segment *>, Segment::Source)));
  connect(this, SIGNAL(grabSpeechSegments(QList<Segment *>, Segment::Source)), m_movieMonitor, SLOT(getSpeechSegments(QList<Segment *>, Segment::Source)));
  connect(this, SIGNAL(grabRefSpeakers(const QStringList &)), m_movieMonitor, SLOT(getRefSpeakers(const QStringList &)));
  connect(this, SIGNAL(showSpkNet(QList<QList<SpeechSegment *> >)), m_movieMonitor, SLOT(viewSpkNet(QList<QList<SpeechSegment *> >)));
  connect(this, SIGNAL(showNarrChart(QList<QList<SpeechSegment *> >)), m_movieMonitor, SLOT(viewNarrChart(QList<QList<SpeechSegment *> >)));
  connect(this, SIGNAL(updateSpkNet(int)), m_movieMonitor, SLOT(updateSpkView(int)));
  connect(m_movieMonitor, SIGNAL(playbackSegments(QList<QPair<qint64, qint64>>)), this, SLOT(playSegments(QList<QPair<qint64, qint64>>)));
  connect(this, SIGNAL(currSubtitle(int)), m_movieMonitor, SLOT(currSubtitle(int)));
  connect(this, SIGNAL(releasePos(bool)), m_movieMonitor, SLOT(releasePos(bool)));
  connect(this, SIGNAL(displayFaces(const QList<Face> &)), m_view, SLOT(displayFaces(const QList<Face> &)));

  connect(m_playlist, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
  connect(m_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));

  connect(this, SIGNAL(getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)), m_movieMonitor, SLOT(getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)));
}

////////////////
// destructor //
////////////////

VideoPlayer::~VideoPlayer()
{
}

void VideoPlayer::reset()
{
  m_player->setMedia(QMediaContent());

  m_view->setFixedSize(m_pixmapItem->pixmap().size());
  m_pixmapItem->show();
  m_videoItem->hide();

  m_controls->reset();
  update();
}

///////////
// slots //
///////////

void VideoPlayer::grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots)
{
  emit getSnapshots(snapshots);
}

void VideoPlayer::addMedia(const QString &fName)
{
  m_playlist->addMedia(QMediaContent(QUrl::fromLocalFile(fName)));
}

void VideoPlayer::insertMedia(int index, const QString &fName)
{
  m_playlist->insertMedia(index, QMediaContent(QUrl::fromLocalFile(fName)));
}

void VideoPlayer::setPlayer(const QString &fName, int index, const QSize &resolution)
{
  if (index != m_currIndex) {
    m_currIndex = index;
    m_playlist->setCurrentIndex(index);
    m_player->setNotifyInterval(40);
  }

  m_vignetteWidget->setVideoCapture(fName);

  if (m_view->size() != resolution || m_videoItem->size() != resolution) {
    m_view->setFixedSize(resolution);
    m_videoItem->setSize(resolution);
  }

  if (m_pixmapItem->isVisible()) {
    m_pixmapItem->hide();
    m_videoItem->show();
    m_controls->setEnabled(true);
  }
}

void VideoPlayer::setPlayerPosition(qint64 position)
{
  if (m_player->state() == QMediaPlayer::PausedState || m_player->state() == QMediaPlayer::StoppedState) {
    m_player->pause();
    m_player->setPosition(position);
  }
}

void VideoPlayer::positionChanged(qint64 position)
{
  if (m_player->state() == QMediaPlayer::PlayingState) {

    // playing current segment
    if (position < m_segments[m_segIdx].second) {
      emit positionUpdated(position);
      emit releasePos(false);
    }

    // end of segment
    else {
      m_player->setPosition(m_segments[m_segIdx].second);
      m_segIdx++;

      // no more segment to play
      if (m_segIdx == m_segments.size()) {
	m_segments.clear();
	m_segments.push_back(QPair<qint64, qint64>(0, m_player->duration()));
	m_segIdx = 0;

	m_player->pause();
	m_controls->playClicked();
	emit releasePos(true);

	// move to (possible) next episode in case of summary playback
	if (m_summIdx > -1) {
	  m_summIdx++;
	  if (m_summIdx < m_summary.size()) {
	    m_currIndex = m_summary[m_summIdx].first;
	    m_playlist->setCurrentIndex(m_currIndex);
	  }
	  else
	    m_summIdx = - 1;
	}
      }
      
      // playing following segments
      else {
	m_player->play();
	m_player->setPosition(m_segments[m_segIdx].first);
	emit releasePos(false);
	m_player->play();
      }
    }
  }
}

void VideoPlayer::stateChanged(QMediaPlayer::State state)
{
  if (state == QMediaPlayer::PlayingState)
    emit playerPaused(false);
  else if (state == QMediaPlayer::PausedState)
    emit playerPaused(true);
}

void VideoPlayer::positionManuallyChanged(qint64 position)
{
  m_player->pause();
  m_player->setPosition(position);
  emit positionUpdated(position);
}

void VideoPlayer::activeHisto(bool dispHist)
{
  emit showHisto(dispHist);
}

void VideoPlayer::currentShot(QList<qint64> positionList)
{
  emit updateVignette(positionList);
}

void VideoPlayer::showVignette(bool showed)
{
  if (showed) {
    connect(this, SIGNAL(updateVignette(QList<qint64>)), m_vignetteWidget, SLOT(updateVignette(QList<qint64>)));
    m_vignetteWidget->show();
  }
  else {
    disconnect(this, SIGNAL(updateVignette(QList<qint64>)), m_vignetteWidget, SLOT(updateVignette(QList<qint64>)));
    m_vignetteWidget->hide();
  }
}

void VideoPlayer::displaySubtitle(SpeechSegment *speechSegment)
{
  m_currSpeechSegment = speechSegment;
  QFont font("Helvetica", 10.5, QFont::Bold);
  m_subEditLabel->setFont(font);
  m_subEditLabel->setText(speechSegment->getText());
  m_subEditLabel->setAlignment(Qt::AlignCenter);
}

void VideoPlayer::displayFaceList(const QList<Face> &faces)
{
  emit displayFaces(faces);
}

void VideoPlayer::setSubReadOnly(bool readOnly)
{
  m_subEditLabel->setReadOnly(readOnly);
}

void VideoPlayer::subTextChanged()
{
  QString currText = m_subEditLabel->toPlainText();
  currText.replace("\n", "<br />");

  if (currText != "" &&
      currText != m_currText &&
      currText != m_currSpeechSegment->getText()) {

    m_currText = currText;
    m_currSpeechSegment->setText(m_currText);
    m_subEditLabel->setAlignment(Qt::AlignCenter);
  }
}

void VideoPlayer::showSegmentation(bool checked, bool annot)
{
  emit viewSegmentation(checked, annot);
}

void VideoPlayer::viewUtteranceTree(bool checked)
{
  emit showUtteranceTree(checked);
}

void VideoPlayer::setDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments)
{
  emit initDiarData(E, Sigma, W, speechSegments);
}

void VideoPlayer::getCurrentPattern(const QPair<int, int> &lsuSpeechBound)
{
  emit grabCurrentPattern(lsuSpeechBound);
}

void VideoPlayer::viewSpeakers(bool checked)
{
  emit showSpeakers(checked);
}

void VideoPlayer::viewSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  emit showSpkNet(sceneSpeechSegments);
}

void VideoPlayer::viewNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  emit showNarrChart(sceneSpeechSegments);
}

void VideoPlayer::updateSpkView(int iScene)
{
  emit updateSpkNet(iScene);
}

void VideoPlayer::getShots(QList<Segment *> shots, Segment::Source source)
{
  emit grabShots(shots, source);
}

void VideoPlayer::getSpeechSegments(QList<Segment *> speechSegments, Segment::Source source)
{
  emit grabSpeechSegments(speechSegments, source);
}

void VideoPlayer::getRefSpeakers(const QStringList &speakers)
{
  emit grabRefSpeakers(speakers);
}

void VideoPlayer::playSegments(QList<QPair<qint64, qint64> > utterances)
{
  if (m_player->state() == QMediaPlayer::PausedState) {
    m_segments = utterances;
    m_segIdx = 0;

    m_player->setPosition(m_segments[0].first);
    m_player->play();
    m_controls->playClicked();
  }
}

void VideoPlayer::setSummary(const QList<QPair<int, QList<QPair<qint64, qint64> > > > &summary)
{
  m_summary = summary;
}

void VideoPlayer::playSummary()
{
  m_summIdx = 0;
  if (m_summIdx < m_summary.size()) {
    m_currIndex = m_summary[m_summIdx].first;
    m_playlist->setCurrentIndex(m_currIndex);
  }
}

void VideoPlayer::currentSubtitle(int subIdx)
{
  emit currSubtitle(subIdx);
}

void VideoPlayer::processFrame(QVideoFrame frame)
{
  QImage image = Convert::fromYuvToRgb(frame);
}

void VideoPlayer::currentIndexChanged(int index)
{
  m_currIndex = index;
  emit episodeIndexChanged(index);
}

void VideoPlayer::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
  if (m_summIdx != -1 && status == QMediaPlayer::BufferedMedia)
    playSegments(m_summary[m_summIdx].second);
}
