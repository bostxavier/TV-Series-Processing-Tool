#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QAbstractItemModel>
#include <QSize>
#include <QMap>

#include <armadillo>

#include "Series.h"
#include "Segment.h"
#include "Episode.h"
#include "Scene.h"
#include "Shot.h"
#include "SpeechSegment.h"
#include "VideoFrame.h"
#include "MovieAnalyzer.h"
#include "SpkDiarizationDialog.h"
#include "FaceDetectDialog.h"
#include "SpkInteractDialog.h"
#include "Face.h"

class ProjectModel: public QAbstractItemModel
{
  Q_OBJECT

 public:
  
  ////////////////////
  // model handling //
  ////////////////////

  ProjectModel(QObject *parent = 0);
  ~ProjectModel();

  bool save(const QString &fName);
  bool load(const QString &fName);

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex());
  bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());
  QModelIndex indexFromSegment(Segment *segment) const;
  int getDepth() const;

  void initEpisodes();
  void initEpisodes_aux(Segment *segment, QList<Episode *> &episodes);
  void setModel(const QString &name, const QString &seriesName, int seasNbr, int epNbr, const QString &epName, const QString &epFName);
  bool addNewEpisode(int seasNbr, int epNbr, const QString &epName, const QString &epFName);
  bool addModel(const QString &projectName, const QString &secProjectFName);

  bool insertSubtitles(const QString &subFName);

  ///////////////////////
  // get specific data //
  ///////////////////////
  
  void retrieveShots(Segment *segment, QList<Shot *> &shots);
  QStringList getSeasons();
  void getSeasons_aux(Segment *segment, QStringList &seasons);
  QStringList retrieveRefSpeakers();
  QList<QStringList> getSeasonSpeakers();
  void getSeasonSpeakers_aux(Segment *segment, QList<QStringList> &seasonSPeakers);

  //////////////////////
  // image processing //
  //////////////////////

  void extractShots(QString fName, int histoType, int nVBins, int nHBins, int nSBins, int metrics, qreal threshold1, qreal threshold2, int nVBlock, int nHBlock);
  qreal evaluateShotDetection(bool displayResults, qreal thresh1, qreal thresh2) const;
  void labelSimilarShots(QString fName, int histoType, int nVBins, int nHBins, int nSBins, int metrics, qreal maxDist, int windowSize, int nVBlock, int nHBlock);
  qreal evaluateSimShotDetection(bool displayResults, qreal thresh1, qreal thresh2) const;
  void extractScenes(Segment::Source vSrc, const QString &fName);
  qreal evaluateSceneDetection(bool displayResults) const;
  void faceDetection(const QString &fName, FaceDetectDialog::Method method, int minHeight);

  //////////////////////
  // audio processing //
  //////////////////////

  void getDiarData();
  bool localSpkDiar(SpkDiarizationDialog::Method method, UtteranceTree::DistType dist, bool norm, UtteranceTree::AgrCrit agr, UtteranceTree::PartMeth partMeth, bool weight, bool sigma);
  bool globalSpkDiar();
  void spkInteract(SpkInteractDialog::InteractType type, SpkInteractDialog::RefUnit unit, int nbDiscards, int interThresh);
  QPair<QVector<qreal>, QVector<qreal> > evaluateSpkInteract(QList<QList<SpeechSegment *> > unitSpeechSegments, bool displayResults, SpkInteractDialog::InteractType type, const QList<QMap<QString, QMap<QString, qreal> > > &conversNets);
  QMap<QString, QMap<QString, qreal> > getSpkOrientedNet(QList<QList<SpeechSegment *> > unitSpeechSegments, SpkInteractDialog::InteractType type);
  QMap<QString, QMap<QString, qreal> > getUttOrientedNet(QList<QList<SpeechSegment *> > unitSpeechSegments, SpkInteractDialog::InteractType type, bool nbWeight = false);
  void musicTracking(int frameRate, int mtWindowSize, int mtHopSize, int chromaStaticFrameSize, int chromaDynamicFrameSize);

  ////////////////////////////
  // audio/video clustering //
  ////////////////////////////
  
  bool coClustering(const QString &fName);

  //////////////////////
  // high-level tasks //
  //////////////////////

  bool summarization(SummarizationDialog::Method method, int seasonNb, const QString &speaker, int dur, qreal granu);
  void showNarrChart(bool checked);

  ///////////////
  // accessors //
  ///////////////

  QString getName() const;
  QString getBaseName() const;
  QString getSeriesName() const;

  public slots:

    ////////////////////
    // model handling //
    ////////////////////
    
    void setEpisode(Episode *currEpisode);
    void retrieveLsuContents(Segment *segment, QList<QList<Shot *> > &lsuShots, QList<QList<SpeechSegment *> > &lsuSpeechSegments, qint64 minDur, qint64 maxDur);
    void setLsuContents(Segment::Source source);
    void setLsuShots(Episode *episode, QList<QList<Shot *> > &lsuShots, Segment::Source source, qint64 minDur = -1, qint64 maxDur = -1, bool rec = false, bool sceneBound = false);
    void setLsuSpeechSegments(Episode *episode, QList<QList<SpeechSegment *> > &lsuSpeechSegments, QList<QList<Shot *> > &lsuShots);
    void insertScene(qint64 position, Segment::Source);
    void initShotAnnot(bool checked);
    void insertSegment(Segment *segment, Segment::Source);
    void removeSegment(Segment *segment);

    ///////////////////
    // view handling //
    ///////////////////
    
    void getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);
    void processSegmentation(bool autoChecked, bool refChecked, bool annot);
    void retrieveSpeakersNet(Segment *segment);
    void updateSpeakersNet(Segment *segment);
    void retrieveSpeakersNet_aux(Segment *segment, QList<QList<SpeechSegment *> > &sceneSpeechSegments, QList<Scene *> &scenes, QVector<QPair<int, int> > sel = QVector<QPair<int, int> >(), bool lsu = false);
    void retrieveEpisodeNetworks(Segment *segment, QList<QMap<QString, QMap<QString, qreal> > > &networks);
    void retrieveSceneNetworks(Segment *segment, QList<QMap<QString, QMap<QString, qreal> > > &networks);
    void setCurrSpeechSeg(qint64 position);
    void setCurrShot(qint64 position);
    void getCurrFaces(qint64 position);
    void getCurrLSU(qint64 position);

    //////////////////////
    // image processing //
    //////////////////////

    void insertShotAuto(qint64 position, qint64 end);
    void appendFaces(qint64 position, const QList<QRect> &faces);
    void externFaceDetection(const QString &fName);

    //////////////////////
    // audio processing //
    //////////////////////

    void setSpkDiarData(const arma::mat &X, const arma::mat &Sigma, const arma::mat &W);
    void insertMusicRate(QPair<qint64, qreal> musicRate);
    void playSegments(QList<QPair<Episode *, QPair<qint64, qint64> > > segments);

    ///////////////////////////
    // export data into file //
    ///////////////////////////

    void exportShotBound(const QString &fName);

 signals:

    ///////////////////////////////
    // update views and monitors //
    ///////////////////////////////

    void grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);

    void addMedia(const QString &fName);
    void insertEpisode(int index, Episode *episode);
    void insertMedia(int index, const QString &fName);
    void setEpisodes(QList<Episode *> episodes);

    void updateEpisode(Episode *episode);
    void positionChanged(qint64 position);
    void displaySubtitle(SpeechSegment *speechSegment);
    void displayFaces(const QList<Face> &faces);
    void playLsus(QList<QPair<Episode *, QPair<qint64, qint64> > > segments);

    void viewSegmentation(bool checked, bool annot);
    void resetSegmentView();
    void resetSpkDiarMonitor();
    void setDepthView(int depth);
    
    void viewSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void viewNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    void updateSpkView(int iScene);

    void getCurrentPattern(const QPair<int, int> &lsuSpeechBound);

    //////////////////////////////
    // update evaluation scores //
    //////////////////////////////

    void truePositive(qint64 position);
    void falsePositive(qint64 position);

    ////////////////////////////////////////////////////
    // send shots speech segments for current episode //
    //  send reference speakers for the whole series  //
    ////////////////////////////////////////////////////
    
    void getShots(QList<Segment *> shots, Segment::Source source);
    void getSpeechSegments(QList<Segment *> speechSegments, Segment::Source source);
    void getRefSpeakers(const QStringList &speakers);
    
    void setDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W);
    void setDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QMap<QString, QList<QPair<qreal, qreal> > > speakers);
    void initDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments);
    

 private:
    
    ////////////////////
    // model handling //
    ////////////////////

    void insertEpisodes(Segment *segment);
    void insertEpisode(Segment *episode);
    void setShotsToManual(Segment *segment);
    void insertVideoFrames(Episode *currEpisode);
    void insertVideoFrames_aux(Segment *segment, qreal fps);
    void removeVideoFrames(Segment *currEpisode);
    void removeAutoShots(Segment *segment);
    void setSegmentsToInsert(QList<Segment *> segmentsToInsert);
    void resetAutoCameraLabels(Segment *segment);
    void resetShotFaces(Segment *segment);

    ////////////////////////
    // evaluation metrics //
    ////////////////////////

    qreal computePrecision(int tp, int fp) const;
    qreal computeRecall(int tp, int fn) const;
    qreal computeFScore(qreal precision, qreal recall) const;
    qreal computeAccuracy(int tp, int fp, int fn, int tn) const;
    void retrieveSimCamLabels(Segment *segment, QList<int> &autCamLabels, QList<int> &manCamLabels) const;

    ///////////////////////
    // get specific data //
    ///////////////////////

    void retrieveSpeechSegments(Episode *episode, QList<SpeechSegment *> &speechSegments);
    void retrieveRefSpeakers_aux(Segment *segment, QMap<QString, qreal> &refSpeakers);
    void retrieveScenePositions(Segment *segment, QList<qint64> &scenePositions) const;
    
    void displayStats(QList<QList<SpeechSegment *> > sceneSpeechSegments);
    arma::vec computeSpkSceneDist(QList<QList<SpeechSegment *> > sceneSpeechSegments, int nBins = -1);
    QVector<QPair<int, int> > getEpisodeRepr(const QVector<QPair<int, int> > &episodes);
    void getBestSubset(const QVector<QPair<int, int> > &episodes, QVector<QPair<int, int> > &currSubset, int k, int i, arma::vec HA, qreal &minDist, QVector<QPair<int, int> > &bestSubset);

    void processErrorCases(QList<QList<SpeechSegment *> > unitSpeechSegments, const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const;
    qreal jaccardIndex(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const;
    qreal cosineSim(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const;
    qreal l2Dist(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const;
    arma::mat vectorize(const QList<QMap<QString, QMap<QString, qreal> > > &inter) const;
    int cardInter(const QStringList &L1, const QStringList &L2) const;
    int cardUnion(const QStringList &L1, const QStringList &L2) const;

    /*************/
    /* TO FILTER */
    /*************/
    void retrieveShotPositions(Segment *segment, QList<qint64> &shotPositions) const;
    void retrieveShotBound(Segment *segment, QList<QPair<int, int> > &shotBound) const;
    void retrieveShotLabels(Segment *segment, QList<QString> &shotLabels, Segment::Source source) const;
    arma::mat genWMat(QMap<QString, QList<int>> spkIdx, const arma::mat &X);

    QString m_name;
    QString m_baseName;
    Series *m_series;
    Episode *m_episode;
    SpeechSegment *m_currSpeechSeg;
    Shot *m_currShot;
    QList<QList<Shot *> > m_lsuShots;
    QList<QList<SpeechSegment *> > m_lsuSpeechSegments;
    QList<Segment *> m_segmentsToInsert;
    QList<Scene *> m_scenes;
    
    MovieAnalyzer *m_movieAnalyzer;

    QList<QPair<qint64, qint64> > m_subBound;
    QMap<QString, QList<QPair<int, qreal> > > m_shotUtterances;
    QMap<QString, QList<QPair<int, qreal> > > m_shotPatterns;
    QMap<QString, QList<QPair<qint64, qint64> > > m_shotPattBound;
    QMap<QString, QList<QPair<qint64, qint64> > > m_strictShotPattBound;
    QList<QString> m_subRefLbl;
    QList<QString> m_subText;

    arma::mat m_W;
};

#endif
