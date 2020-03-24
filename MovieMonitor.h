#ifndef MOVIEMONITOR_H
#define MOVIEMONITOR_H

#include <QWidget>
#include <QVideoFrame>
#include <QScrollArea>
#include <QSlider>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <armadillo>

#include "HistoMonitor.h"
#include "SegmentMonitor.h"
#include "SocialNetWidget.h"
#include "SocialNetMonitor.h"
#include "SpkDiarMonitor.h"
#include "NarrChartMonitor.h"

class MovieMonitor: public QWidget
{
  Q_OBJECT

 public:
  MovieMonitor(QWidget *parent = 0);

  public slots:
    void getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);
    void processFrame(QVideoFrame frame);
    void showHisto(bool dispHist);
    void viewSegmentation(bool checked, bool annot);
    void viewSpeakers(bool checked);
    void viewUtteranceTree(bool checked);
    void setDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments);
    void grabCurrentPattern(const QPair<int, int> &lsuSpeechBound);
    void getShots(QList<Segment *> shots, Segment::Source source);
    void getSpeechSegments(QList<Segment *> speechSegments, Segment::Source source);
    void getRefSpeakers(const QStringList &speakers);
    void positionChanged(qint64 position);
    void playSegments(QList<QPair<qint64, qint64> > utterances);
    void durationChanged(qint64 duration);
    void viewSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void viewNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void updateSpkView(int iScene);
    void currSubtitle(int subIdx);
    void releasePos(bool released);
    
 signals:
    void grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);
    void processMat(const cv::Mat &bgrMat);
    void shotsGrabbed();
    void grabShots(QList<Segment *> shots, Segment::Source source);
    void grabSpeechSegments(QList<Segment *> speechSegments, Segment::Source source);
    void grabRefSpeakers(const QStringList &speakers);
    void updatePosition(qint64 position);
    void playbackSegments(QList<QPair<qint64, qint64>> utterances);
    void updateDuration(qint64 duration);
    void showSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void showNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void updateSpkNet(int iScene);
    void initDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments);
    void getCurrentPattern(const QPair<int, int> &lsuSpeechBound);
    void currentSubtitle(int subIdx);
    void releasePosition(bool released);

 private:
    HistoMonitor *m_histoMonitor;
    SegmentMonitor *m_segmentMonitor;
    SocialNetMonitor *m_socialNetMonitor;
    SpkDiarMonitor *m_spkDiarMonitor;
    QScrollArea *m_scrollArea;
    QSlider *m_segmentSlider;
    NarrChartMonitor *m_narrChartMonitor;
};

#endif
