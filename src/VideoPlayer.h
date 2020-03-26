#ifndef PLAYER_H
#define PLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QTextEdit>
#include <QVideoProbe>
#include <QVideoFrame>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>

#include <armadillo>

#include "VignetteWidget.h"
#include "GraphicsView.h"
#include "PlayerControls.h"
#include "MovieMonitor.h"
#include "Segment.h"
#include "Episode.h"
#include "Face.h"

class VideoPlayer: public QWidget
{
  Q_OBJECT

 public:
  VideoPlayer(int nVignettes = 5, QWidget *parent = 0);
  ~VideoPlayer();
  void reset();

  public slots:
    void grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);
    void addMedia(const QString &fName);
    void insertMedia(int index, const QString &fName);
    void setPlayer(const QString &fName, int index, const QSize &resolution);
    void setPlayerPosition(qint64 position);
    void positionChanged(qint64 position);
    void stateChanged(QMediaPlayer::State state);
    void positionManuallyChanged(qint64 position);
    void activeHisto(bool dispHisto);
    void currentShot(QList<qint64> positionList);
    void showVignette(bool showed);
    void displaySubtitle(SpeechSegment *speechSegment);
    void displayFaceList(const QList<Face> &faces);
    void showSegmentation(bool checked, bool annot);
    void viewUtteranceTree(bool checked);
    void setDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments);
    void getCurrentPattern(const QPair<int, int> &speechSegments);
    void viewSpeakers(bool checked);
    void getShots(QList<Segment *> shots, Segment::Source source);
    void getSpeechSegments(QList<Segment *> speechSegments, Segment::Source source);
    void getRefSpeakers(const QStringList &speakers);
    void playSegments(QList<QPair<qint64, qint64>> utterances);
    void setSummary(const QList<QPair<int, QList<QPair<qint64, qint64> > > > &summary);
    void playSummary();
    void viewSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void viewNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void updateSpkView(int iScene);
    void currentSubtitle(int subIdx);
    void processFrame(QVideoFrame frame);
    void setSubReadOnly(bool readOnly);
    void subTextChanged();
    void currentIndexChanged(int index);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);

 signals:
    void getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);
    void positionUpdated(qint64 position);
    void showHisto(bool dispHist);
    void updateVignette(QList<qint64> positionList);
    void playerPaused(bool pause);
    void viewSegmentation(bool checked, bool annot);
    void showUtteranceTree(bool checked);
    void initDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments);
    void grabCurrentPattern(const QPair<int, int> &lsuSpeechBound);
    void showSpeakers(bool checked);
    void grabShots(QList<Segment *> shots, Segment::Source source);
    void grabSpeechSegments(QList<Segment *> speechSegments, Segment::Source source);
    void grabRefSpeakers(const QStringList &speakers);
    void showSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void showNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void updateSpkNet(int iScene);
    void currSubtitle(int subIdx);
    void releasePos(bool released);
    void displayFaces(const QList<Face> &faces);
    void episodeIndexChanged(int index);

 private:
    QMediaPlayer *m_player;
    QMediaPlaylist *m_playlist;
    int m_currIndex;
    VignetteWidget *m_vignetteWidget;
    GraphicsView *m_view;
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    QGraphicsVideoItem *m_videoItem;
    QTextEdit *m_subEditLabel;
    PlayerControls *m_controls;
    MovieMonitor *m_movieMonitor;
    QVideoProbe *m_probe;
    QList<QPair<qint64, qint64> > m_segments;
    int m_segIdx;
    int m_summIdx;
    QString m_currText;
    SpeechSegment *m_currSpeechSegment;
    QList<QPair<int, QList<QPair<qint64, qint64> > > > m_summary;
};

#endif
