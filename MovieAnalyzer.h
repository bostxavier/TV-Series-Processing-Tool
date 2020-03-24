#ifndef MOVIEANALYZER_H
#define MOVIEANALYZER_H

#include <QWidget>
#include <QString>
#include <QSize>
#include <QProcess>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include "opencv2/videoio.hpp"

#include <armadillo>

#include "VideoFrameProcessor.h"
#include "TextProcessor.h"
#include "AudioProcessor.h"
#include "SocialNetProcessor.h"
#include "Optimizer.h"
#include "Segment.h"
#include "Episode.h"
#include "Shot.h"
#include "VideoFrame.h"
#include "UtteranceTree.h"
#include "SpkDiarMonitor.h"
#include "SpkDiarizationDialog.h"
#include "FaceDetectDialog.h"
#include "SummarizationDialog.h"

class MovieAnalyzer: public QWidget
{
  Q_OBJECT

 public:
  MovieAnalyzer(QWidget *parent = 0);
  ~MovieAnalyzer();

  ////////////////////////////
  // image processing tasks //
  ////////////////////////////

  QSize getResolution(const QString &fName);
  qreal getFps(const QString &fName);
  bool extractShots(const QString &fName,
		    int histoType = 2,
		    int nVBins = 64,
		    int nHBins = 24,
		    int nSBins = 8,
		    int metrics = 0,
		    qreal threshold1 = 30,
		    qreal threshold2 = 20,
		    int nVBlock = 5,
		    int nHBlock = 6,
		    bool viewProgress = true);
  bool labelSimilarShots(QString fName, int histoType, int nVBins, int nHBins, int nSBins, int metrics, qreal maxDist, int windowSize, QList<Shot *> shots, int nVBlock, int nHBlock, bool viewProgress);
  bool faceDetectionOpenCV(QList<Shot *> shots, const QString &fName, int minHeight);
  void faceDetectionZhu(QList<Shot *> shots, const QString &fName, int minHeight);

  ////////////////////////////
  // audio processing tasks //
  ////////////////////////////

  bool localSpkDiarHC(UtteranceTree::DistType dist, bool norm, UtteranceTree::AgrCrit agr, UtteranceTree::PartMeth partMeth, bool weight, bool sigma, QList<SpeechSegment *> speechSegments, QList<QList<SpeechSegment *> > lsuSpeechSegments);
  bool globalSpkDiar(const QString &baseName, QList<QPair<qint64, qint64> > &subBound, QList<QString> &refLbl);
  void setCoOccurrInteract(QList<QList<SpeechSegment *> > unitSpeechSegments, int nbDiscards);
  void setSequentialInteract(QList<QList<SpeechSegment *> > unitSpeechSegments, int nbDiscards, int interThresh, const QVector<bool> &rules);
  void musicTracking(const QString &epFName, int frameRate, int mtWindowSize, int mtHopSize, int chromaStaticFrameSize, int chromaDynamicFrameSize);

  //////////////////////////////////////////////
  // multi-modal audio/video processing tasks //
  //////////////////////////////////////////////

  bool coClustering(const QString &fName, QList<Shot *> shots, QList<QList<Shot *> > lsuShots, QList<SpeechSegment *> speechSegments, QList<QList<SpeechSegment *> > lsuSpeechSegments);
  bool extractScenes(QString fName, const QList<QString> &shotLabels, const QList<qint64> &shotPositions, const QList<QString> &utterLabels, const QList<QPair<qint64, qint64> > &utterBound);

  //////////////////////
  // high-level tasks //
  //////////////////////

  void summarization(SummarizationDialog::Method method, int seasonNb, const QString &speaker, int dur, qreal granu, QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments);

  void exportToFile(const QList<QPair<Episode *, QPair<qint64, qint64> > > summary, QString speaker, int seasNbr, SummarizationDialog::Method method, const QList<int> &steps);

  QList<QPair<Episode *, QPair<qint64, qint64> > > styleSpeakerSummary(QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments, int dur, const QList<qreal> &lsuDuration, const arma::mat &FS, const QMap<int, int> &sceneMapping, const QString &speaker);

  QList<QPair<Episode *, QPair<qint64, qint64> > > randomSpeakerSummary(QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments, int dur, const QList<qreal> &lsuDuration, const arma::mat &FS);

  QList<QPair<Episode *, QPair<qint64, qint64> > > fullSpeakerSummary(QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments, const QList<QPair<QMap<QString, qreal>, QList<int> > > &sceneCovering, const QString &speaker, const QList<qreal> &lsuDuration, const arma::mat &FS, int dur, qreal styleWeight, QList<int> &steps, const QMap<int, int> &sceneMapping);

  qreal getAvgDuration(const QList<int> &lsus, const QList<qreal> &lsuDuration);

  qreal getTotalSceneDuration(const QString &speaker, QList<QList<SpeechSegment *> > sceneSpeechSegments);

  void filterLsus(const QString &speaker, QList<QList<Shot *> > &lsuShots, QList<QList<SpeechSegment *> > &lsuSpeechSegments);

  void displayLsuContent(QList<Shot *> lsuShots, QList<SpeechSegment *> lsuSpeechSegments);

  QList<qreal> getLsuDuration(QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments);
  QPair<qint64, qint64> getLsuBound(int i, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments);
  QList<qreal> getLsuShotSize(QList<QList<Shot *> > lsuShots, bool diff = false);
  QList<qreal> getLsuMusicRate(QList<QList<Shot *> > lsuShots, bool diff = false);
  qreal getLsuSocialRelevance(const QMap<QString, QMap<QString, qreal> > &lsuInteractions, const QString &speaker, const QMap<QString, qreal> &socialState);

  arma::mat computeLsuFormalSimMat(QList<QList<Shot *> > lsuShots);
  qreal computeJaccardSim(QList<Shot *> L1, QList<Shot *> L2);
  
  arma::mat computeLsuSocialSimMat(const QVector<QMap<QString, QMap<QString, qreal> > > &lsuInteractions);
  qreal getLsuSocialSim(const QMap<QString, QMap<QString, qreal> > &lsu1, const QMap<QString, QMap<QString, qreal> > &lsu2);
  arma::mat computeLsuSocialDistMat(const QVector<QMap<QString, QMap<QString, qreal> > > &lsuInteractions);
  qreal getLsuSocialDist(const QMap<QString, QMap<QString, qreal> > &lsu1, const QMap<QString, QMap<QString, qreal> > &lsu2);
  

  arma::vec computeCosts(const QList<qreal> &shotSize, const QList<qreal> &musicRate, const QList<qreal> &socialRelevance, QVector<qreal> lambda);
  arma::vec computeWeights(const QList<qreal> &duration);

  QMap<int, QList<int> > retrieveSceneLsus(QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<SpeechSegment *> > lsuSpeechSegments);
  
  QList<QPair<QMap<QString, qreal>, QList<int> > > splitStoryLine(const QString &speaker, qreal granu, QList<QList<SpeechSegment *> > sceneSpeechSegments, QMap<int, int> &sceneMapping);
  void displayStoryLine(const QList<int> &cIdx, const QList<QPair<int, QList<QPair<QString, qreal > > > > &socialStates, int nbToDisp = 5, bool all = false);

  //////////////////////////////////////////////////////////
  // send data needed for visualizing speaker diarization //
  //////////////////////////////////////////////////////////

  void getSpkDiarData(QList<SpeechSegment *> speechSegments);

  /////////////////////////////////////////////
  // send Logical Story Units video contents //
  /////////////////////////////////////////////
  
  QList<QList<Shot *> > retrieveLsuShots(QList<Shot *> shots, Segment::Source source, qint64 minDur, qint64 maxDur, bool rec = false, bool sceneBound = false);

  /////////////////////////////////////
  // retrieve audio contents by shot //
  /////////////////////////////////////

  QList<QList<SpeechSegment *> > retrieveShotSpeechSegments(QList<Shot *> shots, QList<SpeechSegment *> speechSegments, bool includeAll = false);

  //////////////////////////////
  // filtering audio contents //
  //////////////////////////////
  
  QList<SpeechSegment *> denoiseSpeechSegments(QList<SpeechSegment *> speechSegments);
  QList<SpeechSegment *> filterSpeechSegments(QList<SpeechSegment *> speechSegments);

  public slots:
    void setSpeakerPartition(QList<QList<int>> partition);
    void playSpeakers(QList<int> speakers);
    void faceDetectProcessOutput();
    void retrieveFaceBoundaries(int exitCode, QProcess::ExitStatus exitStatus);
    void musicRateRetrieved(QPair<qint64, qreal> features);

    //////////
    // misc //
    //////////

    qreal jaccardIndex(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const;
    qreal cosineSim(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const;
    qreal l2Dist(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const;
    
 signals:
  void setResolution(const QSize &resolution);
  void setFps(qreal fps);
  void insertShot(qint64 position, qint64 end);
  void setCurrShot(qint64 position);
  void appendFaces(qint64 position, const QList<QRect> &frameFaces);
  void insertScene(qint64 position, Segment::Source source);
  void setDiarData(const arma::mat &X, const arma::mat &Sigma, const arma::mat &W);
  void setSpkDiarData(const arma::mat &X, const arma::mat &Sigma, const arma::mat &W);
  void setSpeaker(qint64 start, qint64 end, const QString &speaker, VideoFrame::SpeakerSource source);
  void playLsus(QList<QPair<Episode *, QPair<qint64, qint64> > > segments);
  void setLocalDer(const QString &score);
  void setGlobalDer(const QString &score);
  void externFaceDetection(const QString &fName);
  void insertMusicRate(QPair<qint64, qreal> musicRate);
  void getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);

 private:
  qreal meanDistance(const QVector<qreal> &distance);

  void normalize(arma::mat &E, const arma::mat &CovInv, UtteranceTree::DistType dist);

  qreal retrieveDer(QString fName);

  arma::mat computeDistMat(const arma::mat &S, const arma::mat &SigmaInv, UtteranceTree::
DistType dist);
  qreal computeDistance(const arma::mat &U, const arma::mat &V, const arma::mat &SigmaInv, UtteranceTree::DistType dist);

  arma::mat retrieveUtterMatDist(arma::mat X, arma::mat CovInv, UtteranceTree::DistType dist, QList<SpeechSegment *> lsuSpeechSegments, QList<SpeechSegment *> speechSegments);
  QPair<qreal, qreal> computeSpkError(QList<QList<int> > &partition, QList<SpeechSegment *> speechSegments, QString pattLabel);
  qreal retrieveSSDer(QList<QPair<qreal, qreal> > &localDer);
  bool setShotCorrMatrix(arma::mat &D, const QString &fName, QList<Shot *> shots, int nV, int nH);
  QList<QPair<int, int> > extractLSUs(const arma::umat &Y, bool rec, qint64 minDur, qint64 maxDur, QList<Shot *> shots);
  void extractLSUs_aux(int first, int last, const arma::umat &Y, qint64 maxDur, QList<Shot *> shots, QList<QPair<int, int> >&sceneBound, bool rec);
  arma::umat computeSimShotMatrix(QList<Shot *> shots, Segment::Source source, bool sceneBound);
  arma::umat computeSimUtterMatrix(const QList<QString> &utterLabels);
  arma::mat computeOverlapShotUtterMat(QList<Shot *> lsuShots, QList<SpeechSegment *> lsuSpeechSegments, QList<QList<SpeechSegment *> > shotSpeechSegments);
  QList<QList<QPair<int, qreal> > > retrieveShotUtterances(const QList<qint64> &shotPositions, const QList<QPair<qint64, qint64> > &utterBound, bool includeAll = false);

  QPair<int, int> getLSUSpeechBound(QList<QList<QPair<int, qreal> > > shotUtterances, int firstShot, int lastShot);

  int getNbRefShotClusters(QList<Shot *> shots);
  int getNbRefSpeakers(QList<SpeechSegment *> speechSegments);

  QList<QRect> detectFacesOpenCV(cv::CascadeClassifier &faceCascade, cv::Mat &frame, int minHeight);
  QList<QRect> detectFacesZhu(qint64 position, cv::Mat &frame, int minHeight);

  bool sameSurroundSpeaker(int i, QList<QList<SpeechSegment *> > speechSegments);
  bool interPrevOcc(int i, QList<QList<SpeechSegment *> > speechSegments);
  bool interNextOcc(int i, QList<QList<SpeechSegment *> > speechSegments);
  QPair<arma::mat, QList<QPair<QString, QString> > > vectorize(const QList<QMap<QString, QMap<QString, qreal> > > &inter, bool directed = true) const;

  SpkDiarMonitor *m_treeMonitor;
  cv::VideoCapture m_cap;
  VideoFrameProcessor *m_vFrameProcessor;
  TextProcessor *m_textProcessor;
  AudioProcessor *m_audioProcessor;
  SocialNetProcessor *m_socialNetProcessor;
  Optimizer *m_optimizer;
  QVector<cv::Mat> m_prevLocHisto;
  cv::Mat m_prevGlobHisto;
  QProcess *m_faceDetectProcess;

  QMap<QString, QList<QPair<qreal, qreal> > > m_utterances;
  QList<QString> m_speakers;
  QList<QPair<qint64, qint64> > m_subBound;
  QString m_localDer;
};

#endif
