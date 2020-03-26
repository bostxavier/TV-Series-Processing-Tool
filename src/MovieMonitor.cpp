#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>

#include "Convert.h"
#include "MovieMonitor.h"

#include <QDebug>

using namespace std;
using namespace cv;

MovieMonitor::MovieMonitor(QWidget *parent)
  : QWidget(parent)
{
  // int windowWidth(1000);
  int windowWidth(480);

  m_histoMonitor = new HistoMonitor;
  m_segmentMonitor = new SegmentMonitor;
  m_socialNetMonitor = new SocialNetMonitor;
  m_spkDiarMonitor = new SpkDiarMonitor;
  m_socialNetMonitor = new SocialNetMonitor;
  m_narrChartMonitor = new NarrChartMonitor;

  m_scrollArea = new QScrollArea;
  m_scrollArea->setWidget(m_segmentMonitor);
  m_scrollArea->setFixedSize(windowWidth, 150);

  m_segmentSlider = new QSlider(Qt::Horizontal);
  m_segmentSlider->setMinimum(windowWidth);
  m_segmentSlider->setMaximum(200000);
  m_segmentSlider->setValue(75000);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(m_histoMonitor);
  layout->addWidget(m_scrollArea);
  layout->addWidget(m_segmentSlider);
  layout->addWidget(m_spkDiarMonitor);
  // layout->addWidget(m_socialNetMonitor);
  setLayout(layout);

  m_histoMonitor->hide();
  m_scrollArea->hide();
  m_segmentSlider->hide();
  m_spkDiarMonitor->hide();
  m_socialNetMonitor->hide();
  m_narrChartMonitor->hide();

  connect(this, SIGNAL(grabShots(QList<Segment *>, Segment::Source)), m_segmentMonitor, SLOT(processShots(QList<Segment *>, Segment::Source)));
  connect(this, SIGNAL(grabSpeechSegments(QList<Segment *>, Segment::Source)), m_segmentMonitor, SLOT(processSpeechSegments(QList<Segment *>, Segment::Source)));
  connect(this, SIGNAL(grabRefSpeakers(const QStringList &)), m_segmentMonitor, SLOT(processRefSpeakers(const QStringList &)));
  connect(this, SIGNAL(updatePosition(qint64)), m_segmentMonitor, SLOT(positionChanged(qint64)));
  connect(this, SIGNAL(updateDuration(qint64)), m_segmentMonitor, SLOT(updateDuration(qint64)));
  connect(m_segmentSlider, SIGNAL(valueChanged(int)), m_segmentMonitor, SLOT(setWidth(int)));
  connect(this, SIGNAL(showSpkNet(QList<QList<SpeechSegment *> >)), m_socialNetMonitor, SLOT(viewSpkNet(QList<QList<SpeechSegment *> >)));
  connect(this, SIGNAL(showNarrChart(QList<QList<SpeechSegment *> >)), m_narrChartMonitor, SLOT(viewNarrChart(QList<QList<SpeechSegment *> >)));
  connect(this, SIGNAL(updateSpkNet(int)), m_socialNetMonitor, SLOT(updateSpkView(int)));
  connect(m_segmentMonitor, SIGNAL(playSegments(QList<QPair<qint64, qint64> >)), this, SLOT(playSegments(QList<QPair<qint64, qint64> >)));
  connect(m_spkDiarMonitor, SIGNAL(playSegments(QList<QPair<qint64, qint64> >)), this, SLOT(playSegments(QList<QPair<qint64, qint64> >)));
  connect(this, SIGNAL(initDiarData(const arma::mat &, const arma::mat &, const arma::mat, QList<SpeechSegment *>)), m_spkDiarMonitor, SLOT(setDiarData(const arma::mat &, const arma::mat &, const arma::mat, QList<SpeechSegment *>)));
  connect(this, SIGNAL(releasePosition(bool)), m_spkDiarMonitor, SLOT(releasePosition(bool)));
  connect(this, SIGNAL(grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)), m_narrChartMonitor, SLOT(grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)));
}

///////////
// slots //
///////////

void MovieMonitor::getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots)
{
  emit grabSnapshots(snapshots);
}

void MovieMonitor::showHisto(bool dispHist)
{
  if (dispHist) {
    m_histoMonitor->show();
    connect(this, SIGNAL(processMat(const cv::Mat &)), m_histoMonitor, SLOT(processMat(const cv::Mat &)));
  }
  else {
    m_histoMonitor->hide();
    disconnect(this, SIGNAL(processMat(const cv::Mat &)), m_histoMonitor, SLOT(processMat(const cv::Mat &)));
  }
}

void MovieMonitor::processFrame(QVideoFrame frame)
{
  Mat bgrMat = Convert::fromYuvToBgrMat(frame);
  emit processMat(bgrMat);
}

void MovieMonitor::viewSegmentation(bool checked, bool annot)
{
  if (checked || annot) {
    m_segmentMonitor->setWidth(m_segmentSlider->value());
    m_segmentMonitor->setAnnotMode(annot);
    m_segmentSlider->show();
    m_scrollArea->show();
  }
  else {
    m_segmentMonitor->reset();
    m_scrollArea->hide();
    m_segmentSlider->hide();
  }
}

void MovieMonitor::viewSpeakers(bool checked)
{
  if (checked) 
    m_socialNetMonitor->show();
  else
    m_socialNetMonitor->hide();
}

void MovieMonitor::viewUtteranceTree(bool checked)
{
  if (checked) {
    connect(this, SIGNAL(getCurrentPattern(const QPair<int, int> &)), m_spkDiarMonitor, SLOT(getCurrentPattern(const QPair<int, int> &)));
    connect(this, SIGNAL(currentSubtitle(int)), m_spkDiarMonitor, SLOT(currentSubtitle(int)));
    m_spkDiarMonitor->show();
  }
  else {
    disconnect(this, SIGNAL(getCurrentPattern(const QPair<int, int> &)), m_spkDiarMonitor, SLOT(getCurrentPattern(const QPair<int, int> &)));
    disconnect(this, SIGNAL(currentSubtitle(int)), m_spkDiarMonitor, SLOT(currentSubtitle(int)));
    m_spkDiarMonitor->hide();
  }
}

void MovieMonitor::setDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments)
{
  emit initDiarData(E, Sigma, W, speechSegments);
}

void MovieMonitor::grabCurrentPattern(const QPair<int, int> &lsuSpeechBound)
{
  emit getCurrentPattern(lsuSpeechBound);
}

void MovieMonitor::getShots(QList<Segment *> shots, Segment::Source source)
{
  emit grabShots(shots, source);
}

void MovieMonitor::getSpeechSegments(QList<Segment *> speechSegments, Segment::Source source)
{
  emit grabSpeechSegments(speechSegments, source);
}

void MovieMonitor::getRefSpeakers(const QStringList &speakers)
{
  emit grabRefSpeakers(speakers);
}

void MovieMonitor::viewSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  emit showSpkNet(sceneSpeechSegments);
}

void MovieMonitor::viewNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  m_narrChartMonitor->hide();

  if (!sceneSpeechSegments.isEmpty())
    emit showNarrChart(sceneSpeechSegments);
}

void MovieMonitor::updateSpkView(int iScene)
{
  emit updateSpkNet(iScene);
}

void MovieMonitor::positionChanged(qint64 position)
{
  m_scrollArea->horizontalScrollBar()->setValue(position *  m_segmentMonitor->getRatio() - m_scrollArea->width() / 2);
  emit updatePosition(position);
}

void MovieMonitor::playSegments(QList<QPair<qint64, qint64> > segments)
{
  emit playbackSegments(segments);
}

void MovieMonitor::durationChanged(qint64 duration)
{
  emit updateDuration(duration);
}

void MovieMonitor::currSubtitle(int subIdx)
{
  emit currentSubtitle(subIdx);
}

void MovieMonitor::releasePos(bool released)
{
  emit releasePosition(released);
}
