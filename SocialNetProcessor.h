#ifndef SOCIALNETPROCESSOR_H
#define SOCIALNETPROCESSOR_H

#include <QObject>
#include <QVector3D>
#include <armadillo>

#include <igraph.h>

#include "Shot.h"
#include "SpeechSegment.h"
#include "Vertex.h"
#include "Edge.h"
#include "Optimizer.h"

class SocialNetProcessor: public QWidget
{
  Q_OBJECT

public:
  enum GraphSource {
    CoOccurr, Interact
  };

  SocialNetProcessor(QWidget *parent = 0);
  ~SocialNetProcessor();

  void setSceneSpeechSegments(QList<QList<SpeechSegment *> > sceneSpeechSegments);
  void setGraph(GraphSource src, bool layout = true);
  void updateGraph(GraphSource src);
  void updateNetView(int iScene);
  void setDirected(bool directed);
  void setWeighted(bool weighted);
  void setNbWeight(bool nbWeight);
  void setInterThresh(int interThresh);
  void setNbDiscard(int nbDiscard);
  
  qreal communityDetection(QVector<QList<QPair<qreal, QString> > > &partition);
  QVector<int> communityDetection_bis();
  QVector<int> communityDetection_ter();
  arma::vec trackSpeakerCommunities(const QString &speaker);
  void analyzeDynCommunities();
  QList<QPair<int, int> > getNarrEpisodes(const QString &speaker, const QList<QPair<int, int> > &communities, int n);
  arma::vec getComNodeStrengths(const QString &speaker, const QList<QPair<int, int> > &communities);
  arma::vec getComWeights(const QList<QPair<int, int> > &communities);
  arma::mat getComSims(const QList<QPair<int, int> > &communities);
  
  void exportGraphToGraphmlFile(const QMap<QString, QMap<QString, qreal> > &edges, const QString &fName);
  void exportGraphToPajekFile(const QMap<QString, QMap<QString, qreal> > &edges, const QString &fName);
  void exportViewToFile(int i, const QString &baseName, const QString &method);

  void displaySceneSpeakers() const;
  QStringList getInteractingSpeakers(const QMap<QString, QMap<QString, qreal> > &interactions) const;

  QVector<QList<Vertex> > getVerticesViews() const;
  QVector<QList<Edge> > getEdgesViews() const;
  QVector<QPair<int, int> > getSceneRefs() const;
  QVector<QMap<QString, QMap<QString, qreal> > > getSceneInteractions() const;
  QVector<QMap<QString, QMap<QString, qreal> > > getNetworkViews() const;

  void displaySpeakerNeighbors(const QString &speaker, int step = 1, int nbToDisp = 4);
  arma::mat getSocialStatesDist(const QString &speaker, QList<QPair<int, QList<QPair<QString, qreal > > > > &socialStates);

  /////////////////////
  // narrative chart //
  /////////////////////

  void setNarrChart();
  void setNarrChart_aux(int inf, int sup, int jMin, const arma::mat &S, const QStringList &speakers, QMap<QString, QVector<QPoint> > &m_narrChart);
  QMap<QString, QVector<QPoint> > getNarrChart() const;
  QVector<int> posFromNarrChart(int index, const QStringList &selSpeakers, QMap<QString, QVector<QPoint> > &narrChart);

  void setNarrChart_aux_bis(int inf, int sup, const QStringList &selSpeakers);
  arma::mat getMatrix(const QStringList &speakers, int i);
  QMap<QString, QMap<QString, qreal> > snapshotToNeighbors(int i);
  QStringList filterSpeakers(int iMin, int iMax, qreal minStrength);

  public slots:
    
private:
  QMap<QString, QMap<QString, qreal> > getCoOccurrGraph();
  QMap<QString, QMap<QString, qreal> > getInteractGraph();
  qreal getAngle(const QVector3D &v1, const QVector3D &v2);
  
  /////////////////////////////////////////////
  // network initialization (graph + widget) //
  /////////////////////////////////////////////

  void initGraph(const QMap<QString, QMap<QString, qreal> > &edges);
  void setGraphEdges(QMap<QString, QMap<QString, qreal> > edges);
  void initGraphWidget();
  QList<Vertex> setVertices();
  QList<Edge> setEdges();
  QList<qreal> getNodeStrengths();
  QList<QVector3D> getLayoutCoord(int nVertices, int nIter, qreal maxdelta, bool use_seed = false, bool twoDim = false);

  ///////////////////////////////////////////////
  // update network from view (graph + widget) //
  ///////////////////////////////////////////////

  void updateGraph(const QMap<QString, QMap<QString, qreal> > &currView);
  void updateGraphWidget(bool layout);
  void updateVertices(bool layout);
  void updateEdges();

  //////////////////////////////////////
  // community detection and matching //
  //////////////////////////////////////

  QVector<arma::vec> vectorizeCommunities(const QVector<int> &comAssign);
  QVector<arma::vec> vectorizeCommunities_bis(const QVector<int> &comAssign);
  void displayCommunities(const QVector<int> &comAssign, int minSize, qreal minStrength = 1.0) const;
  qreal jaccardIndex(const arma::vec &U, const arma::vec &V) const;
  qreal cosineSim(const arma::vec &U, const arma::vec &V) const;
  qreal l2Dist(const arma::vec &U, const arma::vec &V) const;
  QVector<int> partitionMatching() const;
  QVector<int> partitionMatching(const QVector<arma::vec> &prevComVec, const QVector<arma::vec> &currComVec);
  void adjustComLabels(const QList<QList<QPair<int, int> > > &dynCommunities);

  ////////////////////
  // handling views //
  ////////////////////

  void buildNetworkViews(bool layout);
  void updateSceneRefs(int i);

  QVector<QMap<QString, QMap<QString, qreal> > > buildNetworkSnapshots();
  QVector<QMap<QString, QMap<QString, qreal> > > buildNetworkSnapshots_bis();

  bool interactingSpeaker(const QString &spk, const QMap<QString, QMap<QString, qreal> > &network);

  QVector<QMap<QString, QMap<QString, qreal> > > mergeNetworkSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &networkSnapshotsFromPast, const QVector<QMap<QString, QMap<QString, qreal> > > &networkSnapshotsFromFuture);
  QVector<QMap<QString, QMap<QString, qreal> > > buildNetworkSnapshotsFromPast();
  QVector<QMap<QString, QMap<QString, qreal> > > buildNetworkSnapshotsFromFuture();

  qreal sigmoid(qreal x, qreal lambda = 0.01);

  void updateSnapshot(QMap<QString, QMap<QString, qreal> > &snapshot, const QString &fSpeaker, const QString &sSpeaker, qreal weight);

  QMap<QString, QList<QPair<int, qreal> > > getSpeakerOcc(const QVector<QMap<QString, QMap<QString, qreal> > > &inter);
  QMap<QString, QMap<QString, QList<QPair<int, qreal> > > > getInterOcc(const QVector<QMap<QString, QMap<QString, qreal> > > &inter);
  QVector<qreal> getSceneWeights(const QList<QPair<int, qreal> > &interOcc, const QList<QPair<int, qreal> > &fSpeakerOcc, const QList<QPair<int, qreal> > &sSpeakerOcc, int n);
  QVector<qreal> weightEdges(const QVector<qreal> &sceneWeights, qreal lambda) const;

  QVector<QMap<QString, QMap<QString, qreal> > > buildCumNetworks(int timeSlice);

  void normalizeWeights();
  arma::vec monitorRelationWeight(const QString &fSpk, const QString &sSpk, const QVector<QMap<QString, QMap<QString, qreal> > > &netViews);
  arma::vec monitorSpkStrength(const QString &spk, const QVector<QMap<QString, QMap<QString, qreal> > > &netViews);
  void monitorPlot();

  /////////////////////////
  // community detection //
  /////////////////////////

  QList<QList<QPair<int, int> > > getDynCommunities(const QVector<QVector<int> > &communityMatching);
  arma::vec displayDynCommunity(const QList<QPair<int, int> > &dynCommunity, int minSize, int n, qreal minStrength = 1.0);
  QList<Vertex> getStaticCommunity(int i, int comId);
  qreal computeCommunityModularity(const QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges);
  qreal computeCommunityBalance(const QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges);
  qreal computeCommunityWeight(const QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges);
  qreal computeCommunityNodeStrength(const QString &nodeLabel, QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges);

  qreal getSpeakerStrength(const QList<QPair<qreal, QString> > &neighbors);
  QList<QPair<qreal, QString> > getSpeakerNeighbors(const QString &speaker, const QMap<QString, QMap<QString, qreal> > &edges);
  QList<QPair<QString, QString> > getEdgesEnds();
  arma::vec vectorizeNeighbors(const QString &speaker, const QMap<QString, QMap<QString, qreal> > &edges);
  arma::vec simToNeighborsNeighborhood(const QString &speaker, const QMap<QString, QMap<QString, qreal> > &edges);

  QVector<QMap<QString, QMap<QString, qreal> > > m_sceneInteractions;
  QVector<QMap<QString, QMap<QString, qreal> > > m_networkViews;
  QList<QList<SpeechSegment *> > m_sceneSpeechSegments;

  QVector<QPair<int, int> > m_sceneRefs;

  QVector<QList<Vertex> > m_verticesViews;
  QVector<QList<Edge> > m_edgesViews;
  QStringList m_verticesLabels;

  igraph_t m_graph;
  igraph_matrix_t m_coord;
  bool m_directed;
  bool m_weighted;
  bool m_nbWeight;

  int m_iCurrScene;

  QList<Vertex> m_vertices;
  QList<Edge> m_edges;

  QList<QList<QPair<int, int> > > m_dynCommunities;

  QMap<QString, QVector<QPoint> > m_narrChart;

  Optimizer *m_optimizer;
};

#endif
