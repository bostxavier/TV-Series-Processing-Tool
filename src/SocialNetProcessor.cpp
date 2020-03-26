#include <QtMath>
#include <QFile>
#include <QProgressDialog>

#include <QDebug>

#include "SocialNetProcessor.h"
#include "Season.h"
#include "Episode.h"
#include "Optimizer.h"
#include "DendogramWidget.h"

using namespace std;

SocialNetProcessor::SocialNetProcessor(QWidget *parent)
  : QWidget(parent),
    m_directed(false),
    m_weighted(true),
    m_nbWeight(false),
    m_iCurrScene(0)
{
  igraph_i_set_attribute_table(&igraph_cattribute_table);
  m_optimizer = new Optimizer();
}

SocialNetProcessor::~SocialNetProcessor()
{
  igraph_matrix_destroy(&m_coord);
  igraph_destroy(&m_graph);
}

/////////////////////////////////////////////
// network initialization (graph + widget) //
/////////////////////////////////////////////

void SocialNetProcessor::setGraph(SocialNetProcessor::GraphSource source, bool layout)
{
  QMap<QString, QMap<QString, qreal> > edges;

  // clear widget graph
  m_vertices.clear();
  m_edges.clear();

  switch (source) {
  case SocialNetProcessor::CoOccurr:
    edges = getCoOccurrGraph();
    break;
  case SocialNetProcessor::Interact:
    edges = getInteractGraph();
    break;
  }

  // build agglomerative graph and corresponding widget
  initGraph(edges);
  initGraphWidget();

  // possibly initialize layout
  int nVertices = igraph_vcount(&m_graph);
  igraph_matrix_init(&m_coord, nVertices, 3);
  getLayoutCoord(nVertices, 500, nVertices, false, true);

  // build network views
  // monitorPlot();
  buildNetworkViews(layout);
}

void SocialNetProcessor::buildNetworkViews(bool layout)
{
  // set network views
  m_networkViews = buildNetworkSnapshots();
  // m_networkViews = buildCumNetworks(10);
  // m_networkViews = buildCumNetworks(m_networkViews.size());
  
  for (int i(0); i < m_networkViews.size(); i++) {
    // update scene references (season + episode)
    updateSceneRefs(i);
  }
  if (layout) {

    // progress bar
    QProgressDialog progress(tr("Building graph views..."), tr("Cancel"), 0, m_networkViews.size(), this);
    progress.setWindowModality(Qt::WindowModal);

    QVector<arma::vec> prevComVec;
    QVector<QVector<int> > communityMatching(m_networkViews.size() - 1);

    for (int i(0); i < m_networkViews.size(); i++) {

      // update agglomerative graph and corresponding widget
      updateGraph(m_networkViews[i]);
      updateGraphWidget(layout);

      // export current view to file
      // exportViewToFile(i, "BB", "dyn_ns");
      // exportViewToFile(i, "BB", "dyn_ts10");
      // exportViewToFile(i, "BB", "dyn_ts40");
      // exportViewToFile(i, "BB", "cum");
      
      // exportViewToFile(i, "GoT", "dyn_ns");
      // exportViewToFile(i, "GoT", "dyn_ts10");
      // exportViewToFile(i, "GoT", "dyn_ts40");
      // exportViewToFile(i, "GoT", "cum");

      // exportViewToFile(i, "HoC", "dyn_ns");
      // exportViewToFile(i, "HoC", "dyn_ts10");
      // exportViewToFile(i, "HoC", "dyn_ts40");
      // exportViewToFile(i, "HoC", "cum");

      // perform community detection on current graph
      QVector<int> comAssign = communityDetection_ter();

      // assign each node to his own community
      for (int j(0); j < comAssign.size(); j++)
	m_vertices[j].setCom(comAssign[j]);

      // update view vertices and edges
      m_verticesViews[i] = m_vertices;
      m_edgesViews[i] = m_edges;

      // vectorize resulting communities
      QVector<arma::vec> currComVec = vectorizeCommunities(comAssign);
      
      // matching with previous communities
      if( !prevComVec.isEmpty()) {
	QVector<int> matching = partitionMatching(prevComVec, currComVec);
	communityMatching[i-1] = matching;
      }

      prevComVec = currComVec;

      // update progress bar
      progress.setValue(i);
      if (progress.wasCanceled())
	return;
    }

    // make consistent the community labels
    m_dynCommunities = getDynCommunities(communityMatching);
    adjustComLabels(m_dynCommunities);

    // normalize weights
    normalizeWeights();

    // set current view to selected scene
    m_vertices = m_verticesViews[m_iCurrScene];
    m_edges = m_edgesViews[m_iCurrScene];
  }
}

void SocialNetProcessor::updateSceneRefs(int i)
{
  if (!m_sceneSpeechSegments[i].isEmpty()) {
    Episode *currEpisode = static_cast<Episode *>(m_sceneSpeechSegments[i].first()->parent());
    Season *currSeason = static_cast<Season *>(currEpisode->parent());
    m_sceneRefs[i] = QPair<int, int>(currSeason->getNumber(), currEpisode->getNumber());
  }
  else if (i > 0)
    m_sceneRefs[i] = m_sceneRefs[i-1];
}

void SocialNetProcessor::adjustComLabels(const QList<QList<QPair<int, int> > > &dynCommunities)
{
  QMap<int, QMap<int, int> > newComs;

  int currLbl(1);

  for (int i(0); i < dynCommunities.size(); i++)
    if (dynCommunities[i].size() > 1) {
      for (int j(0); j < dynCommunities[i].size(); j++)
	newComs[dynCommunities[i][j].first][dynCommunities[i][j].second] = currLbl;
      currLbl++;
    }

  for (int i(0); i < m_verticesViews.size(); i++) {
    for (int j(0); j < m_verticesViews[i].size(); j++) {
      int prevCom = m_verticesViews[i][j].getCom();
      QString label = m_verticesViews[i][j].getLabel();
      m_verticesViews[i][j].setCom(newComs[i][prevCom]);
    }
  }
}

arma::vec SocialNetProcessor::trackSpeakerCommunities(const QString &speaker)
{
  int step(25);
  arma::vec U;
  U.zeros(m_verticesViews.size());
  QList<QPair<int, int> > communities;

  int minSize(2);
  qreal minStrength(1.0);

  for (int i(0); i < m_verticesViews.size(); i++) {
    
    // retrieve speaker in current snapshot
    int j(0);
    while (j < m_verticesViews[i].size() && m_verticesViews[i][j].getLabel() != speaker) {
      j++;
    }
    
    // speaker found
    if (j < m_verticesViews[i].size()) {
      int com = m_verticesViews[i][j].getCom();

      if (com != -1) {
	QList<Vertex> staticCommunity = getStaticCommunity(i, com);
	qreal mod = computeCommunityModularity(staticCommunity, m_networkViews[i]);
	qreal weight = computeCommunityWeight(staticCommunity, m_networkViews[i]);
	qreal balance = computeCommunityBalance(staticCommunity, m_networkViews[i]);
	qreal comNodeStrength = computeCommunityNodeStrength(speaker, staticCommunity, m_networkViews[i]);

	U(i) = comNodeStrength;

	communities.push_back(QPair<int, int>(i, com));

	std::sort(staticCommunity.begin(), staticCommunity.end());
	std::reverse(staticCommunity.begin(), staticCommunity.end());

	if (!(i % step) && staticCommunity.size() >= minSize) {
	  for (int k(0); k < staticCommunity.size(); k++) {

	    QString spkLbl = staticCommunity[k].getLabel();
	    qreal strength = staticCommunity[k].getWeight();
	
	    if (strength >= minStrength)
	      qDebug() << i << weight << mod << balance << comNodeStrength << com << spkLbl << strength;
	  }
	  qDebug();
	}
      }
    }
  }

  QList<QPair<int, int> > narrEpisodes = getNarrEpisodes(speaker, communities, 8);

  for (int i(0); i < narrEpisodes.size(); i++) {

    QList<Vertex> staticCommunity = getStaticCommunity(narrEpisodes[i].first, narrEpisodes[i].second);
    qreal mod = computeCommunityModularity(staticCommunity, m_networkViews[narrEpisodes[i].first]);
    qreal weight = computeCommunityWeight(staticCommunity, m_networkViews[narrEpisodes[i].first]);
    qreal balance = computeCommunityBalance(staticCommunity, m_networkViews[narrEpisodes[i].first]);
    qreal comNodeStrength = computeCommunityNodeStrength(speaker, staticCommunity, m_networkViews[narrEpisodes[i].first]);

    std::sort(staticCommunity.begin(), staticCommunity.end());
    std::reverse(staticCommunity.begin(), staticCommunity.end());
    
    if (staticCommunity.size() >= minSize) {
      for (int k(0); k < staticCommunity.size(); k++) {

	QString spkLbl = staticCommunity[k].getLabel();
	qreal strength = staticCommunity[k].getWeight();
	
	if (strength >= minStrength)
	  qDebug() << narrEpisodes[i].first << weight << mod << balance << comNodeStrength << narrEpisodes[i].second << spkLbl << strength;
      }
      qDebug();
      // QStringList speakers = getInteractingSpeakers(m_sceneInteractions[narrEpisodes[i].first]);
      // qDebug() << speakers << endl;
    }
  }
  
  return U;
}

void SocialNetProcessor::analyzeDynCommunities()
{
  qDebug() << "# dyn.:" << m_dynCommunities.size();

  // consider n longest dynamic communities
  int n(5);
  arma::mat M;
  M.zeros(n, m_networkViews.size());

  // loop over dynamic communities
  // for (int i(0); i < m_dynCommunities.size(); i++) {
  for (int i(0); i < n; i++) {
    qDebug() << "----" << i << ":" << m_dynCommunities[i].size() << "----" << endl;
    arma::vec U = displayDynCommunity(m_dynCommunities[i], 2, m_networkViews.size());
    M.row(i) = U.t();

    // QList<QPair<int, int> > narrEpisodes = getNarrEpisodes(m_dynCommunities[i], 3);
  }

  M.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/M.dat", arma::raw_ascii);
}

QList<QPair<int, int> > SocialNetProcessor::getNarrEpisodes(const QString &speaker, const QList<QPair<int, int> > &communities, int n)
{
  // list of n episodes to return
  QList<QPair<int, int> > narrEpisodes;

  // arma::vec C = getComWeights(communities);
  arma::vec C = getComNodeStrengths(speaker, communities);
  arma::mat S = getComSims(communities);

  // get n most relevant substories such that redundancy is minimized (greedy approach)
  while (narrEpisodes.size() <= n) {

    // find maximum value and corresponding index
    arma::uword index;
    qreal max = C.max(index);
    
    C(index) = -arma::datum::inf;

    // re-evaluate commununities relevance
    for (arma::uword i(0); i < C.n_rows; i++)
      C(i) -= 2 * S(i, index);

    narrEpisodes.push_back(communities[index]);
  }

  std::sort(narrEpisodes.begin(), narrEpisodes.end());

  qDebug() << narrEpisodes;

  return narrEpisodes;
}

arma::vec SocialNetProcessor::getComNodeStrengths(const QString &speaker, const QList<QPair<int, int> > &communities)
{
  arma::vec C;
  C.zeros(communities.size());

  for (int i(0); i < communities.size(); i++) {

    QList<Vertex> staticCommunity = getStaticCommunity(communities[i].first, communities[i].second);
    qreal nodeComStr = computeCommunityNodeStrength(speaker, staticCommunity, m_networkViews[communities[i].first]);    
    C(i) = nodeComStr;
  }

  return C;
}

arma::vec SocialNetProcessor::getComWeights(const QList<QPair<int, int> > &communities)
{
  arma::vec C;
  C.zeros(communities.size());

  for (int i(0); i < communities.size(); i++) {

    QList<Vertex> staticCommunity = getStaticCommunity(communities[i].first, communities[i].second);
    qreal mod = computeCommunityModularity(staticCommunity, m_networkViews[communities[i].first]);
    
    C(i) = mod;
  }

  return C;
}

arma::mat SocialNetProcessor::getComSims(const QList<QPair<int, int> > &communities)
{
  arma::mat S;
  S.zeros(communities.size(), communities.size());
 
  QVector<arma::vec> vecCommunities(communities.size());

  // vectorize communities
  for (int i(0); i < communities.size(); i++) {
    
    int iScene = communities[i].first;
    int iCom = communities[i].second;

    QList<Vertex> staticCommunity = getStaticCommunity(iScene, iCom);
    QStringList members;

    for (int j(0); j < staticCommunity.size(); j++)
      members.push_back(staticCommunity[j].getLabel());

    arma::vec V;
    V.zeros(m_verticesViews[iScene].size());

    for (int j(0); j < m_verticesViews[iScene].size(); j++)
      if (members.contains(m_verticesViews[iScene][j].getLabel()))
	V(j) = m_verticesViews[iScene][j].getWeight();
    
    vecCommunities[i] = V;
  }

  // compute similarity between any pair of communities
  for (int i(0); i < vecCommunities.size()-1; i++)
    for (int j(i); j < vecCommunities.size(); j++) {

      if (arma::norm(vecCommunities[i]) > 0 && arma::norm(vecCommunities[j]) > 0)
	S(i, j) = cosineSim(vecCommunities[i], vecCommunities[j]);
      else
	S(i, j) = 0;
      
      S(j, i) = S(i, j);
    }

  return S;
}

QList<QList<QPair<int, int> > > SocialNetProcessor::getDynCommunities(const QVector<QVector<int> > &communityMatching)
{
  QList<QList<QPair<int, int> > > dynCommunities;
  QList<QPair<int, int> > explored;


  // looping over scenes
  for (int i(0); i < communityMatching.size(); i++)

    // looping over communities extracted from current view
    for (int j(0); j < communityMatching[i].size(); j++) {

      QPair<int, int> first(i, j);
	
      if (!explored.contains(first)) {

	QList<QPair<int, int> > currDynCommunity;

	QPair<int, int> from = first;
	QPair<int, int> to(i + 1, communityMatching[i][j]);

	while (to.first < communityMatching.size() && to.second != -1) {
	
	  currDynCommunity.push_back(from);
	  explored.push_back(from);

	  from = to;
	  to = QPair<int, int>(to.first + 1, communityMatching[to.first][to.second]);
	}

	if (from != first) {
	  currDynCommunity.push_back(from);
	  explored.push_back(from);
	}

	// insert new dynamic communitiy in proper rank
	if (!currDynCommunity.isEmpty()) {

	  int k(0);
	  int length = currDynCommunity.size();

	  while (k < dynCommunities.size() && length < dynCommunities[k].size())
	    k++;
	  
	  dynCommunities.insert(k, currDynCommunity);
	}
      }
    }

  /*
  for (int i(0); i < dynCommunities.size(); i++)
    if (dynCommunities[i].size() > 100) {
      qDebug() << "-----" << i << "-----";
      displayDynCommunity(dynCommunities[i], 2);
    }
  */
      
  return dynCommunities;
}

arma::vec SocialNetProcessor::displayDynCommunity(const QList<QPair<int, int> > &dynCommunity, int minSize, int n, qreal minStrength)
{
  arma::vec U;
  U.zeros(n);

  int step = dynCommunity.size() / 15;
  if (step == 0)
    step = 1;

  int k(0);
  for (int i(0); i < dynCommunity.size(); i++) {

    QList<Vertex> staticCommunity = getStaticCommunity(dynCommunity[i].first, dynCommunity[i].second);
    qreal mod = computeCommunityModularity(staticCommunity, m_networkViews[dynCommunity[i].first]);
    qreal weight = computeCommunityWeight(staticCommunity, m_networkViews[dynCommunity[i].first]);
    qreal balance = computeCommunityBalance(staticCommunity, m_networkViews[dynCommunity[i].first]);

    U(dynCommunity[i].first) = balance;

    std::sort(staticCommunity.begin(), staticCommunity.end());
    std::reverse(staticCommunity.begin(), staticCommunity.end());

    if (i == k) {
      if (staticCommunity.size() >= minSize) {
	for (int j(0); j < staticCommunity.size(); j++) {

	  QString spkLbl = staticCommunity[j].getLabel();
	  qreal strength = staticCommunity[j].getWeight();
	
	  if (strength >= minStrength)
	    qDebug() << dynCommunity[i].first << weight << mod << balance << dynCommunity[i].second << spkLbl << strength;
	}
	qDebug();

	QStringList speakers = getInteractingSpeakers(m_sceneInteractions[dynCommunity[i].first]);

	qDebug() << speakers << endl;

      }
      k += step;
    }
  }

  return U;
}

qreal SocialNetProcessor::computeCommunityNodeStrength(const QString &nodeLabel, QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges)
{
  qreal comNodeStrength(0.0);
  qreal strength_in(0.0);
  qreal strength_out(0.0);

  QStringList subsetSpeakers;

  // retrieve node labels
  for (int i(0); i < community.size(); i++)
    subsetSpeakers.push_back(community[i].getLabel());

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();

      if (subsetSpeakers.contains(fSpeaker) && subsetSpeakers.contains(sSpeaker) && (fSpeaker == nodeLabel || sSpeaker == nodeLabel))
	strength_in += w;

      else if ((subsetSpeakers.contains(fSpeaker) || subsetSpeakers.contains(sSpeaker)) && (fSpeaker == nodeLabel || sSpeaker == nodeLabel)) {
	strength_out += w;
      }

      it2++;
    }
    it1++;
  }

  return strength_in / (strength_in + strength_out);
}

qreal SocialNetProcessor::computeCommunityModularity(const QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges)
{
  qreal modularity;
  qreal w_in(0.0);
  qreal w_out(0.0);

  QStringList subsetSpeakers;

  // retrieve node labels
  for (int i(0); i < community.size(); i++)
    subsetSpeakers.push_back(community[i].getLabel());

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();

      if (subsetSpeakers.contains(fSpeaker) && subsetSpeakers.contains(sSpeaker))
	w_in += w;
      else if (subsetSpeakers.contains(fSpeaker) || subsetSpeakers.contains(sSpeaker))
	w_out += w;

      it2++;
    }
    it1++;
  }

  modularity = w_in / (w_out + w_in);

  return modularity;
}

qreal SocialNetProcessor::computeCommunityBalance(const QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges)
{
  qreal w_in(0.0);
  qreal w_out(0.0);

  QStringList subsetSpeakers;

  // retrieve node labels
  for (int i(0); i < community.size(); i++)
    subsetSpeakers.push_back(community[i].getLabel());

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();

      if (subsetSpeakers.contains(fSpeaker) && subsetSpeakers.contains(sSpeaker))
	w_in += w;
      else if (subsetSpeakers.contains(fSpeaker) || subsetSpeakers.contains(sSpeaker))
	w_out += w;

      it2++;
    }
    it1++;
  }

  return w_in - w_out;
}

qreal SocialNetProcessor::computeCommunityWeight(const QList<Vertex> &community, const QMap<QString, QMap<QString, qreal> > &edges)
{
  qreal weight(0.0);

  QStringList subsetSpeakers;

  // retrieve node labels
  for (int i(0); i < community.size(); i++)
    subsetSpeakers.push_back(community[i].getLabel());

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();

      if (subsetSpeakers.contains(fSpeaker) && subsetSpeakers.contains(sSpeaker))
	weight += w;

      it2++;
    }
    it1++;
  }

  return weight;
}

QList<Vertex> SocialNetProcessor::getStaticCommunity(int i, int comId)
{
  QList<Vertex> staticCommunity;

  for (int j(0); j < m_verticesViews[i].size(); j++) {
    int com = m_verticesViews[i][j].getCom();
    if (com == comId)
      staticCommunity.push_back(m_verticesViews[i][j]);
  }

  return staticCommunity;
}

QVector<int> SocialNetProcessor::partitionMatching(const QVector<arma::vec> &prevComVec, const QVector<arma::vec> &currComVec)
{
  QVector<int> matching;

  int p(prevComVec.size());
  int n(currComVec.size());

  arma::mat A;
  A.zeros(p, n);

  for (int i(0); i < p; i++)
    for (int j(0); j < n; j++) {

      if (arma::norm(prevComVec[i]) > 0 && arma::norm(currComVec[j]))
	A(i, j) = cosineSim(prevComVec[i], currComVec[j]);
      else
	A(i, j) = 0;
    }

  m_optimizer->optimalMatching_bis(A, matching);

  /*
  for (int i(0); i < matching.size(); i++) {
    if (matching[i] != -1)
      qDebug() << i << matching[i] << A(i, matching[i]);
  }
  */
  
  return matching;
}

QVector<arma::vec> SocialNetProcessor::vectorizeCommunities(const QVector<int> &comAssign)
{
  QVector<arma::vec> comVec;
  QVector<QList<int> > partition;

  // retrieve number of communities
  int nCom(0);
  
  for (int i(0); i < comAssign.size(); i++)
    if (comAssign[i] > nCom)
      nCom = comAssign[i];

  comVec.resize(nCom + 1);
  partition.resize(nCom + 1);

  // create partition
  for (int i(0); i < comAssign.size(); i++)
    partition[comAssign[i]].push_back(i);

  for (int i(0); i < partition.size(); i++) {

    comVec[i].zeros(comAssign.size());

    for (int j(0); j < partition[i].size(); j++) {
      comVec[i](partition[i][j]) = m_vertices[partition[i][j]].getWeight();

      // if (partition.size() > 1 && m_vertices[partition[i][j]].getWeight() > 2)
      // qDebug() << i << partition[i][j] << m_vertices[partition[i][j]].getLabel() << m_vertices[partition[i][j]].getWeight();
    }
  }
  
  return comVec;
}

QVector<arma::vec> SocialNetProcessor::vectorizeCommunities_bis(const QVector<int> &comAssign)
{
  QVector<arma::vec> comVec;
  QVector<QList<int> > partition;

  // retrieve number of communities
  int nCom(0);
  
  for (int i(0); i < comAssign.size(); i++)
    if (comAssign[i] > nCom)
      nCom = comAssign[i];

  comVec.resize(nCom + 1);
  partition.resize(nCom + 1);

  // create partition
  for (int i(0); i < comAssign.size(); i++)
    partition[comAssign[i]].push_back(i);

  // loop over partition
  for (int i(0); i < partition.size(); i++) {

    comVec[i].zeros(m_edges.size());
    
    // loop every pair of nodes in current part
    for (int j(0); j < partition[i].size() - 1; j++) {
      for (int k(j+1); k < partition[i].size(); k++) {
	
	// loop over edges
	int l(0);
	bool found(false);
	
	while (l < m_edges.size() && !found) {
	  int v1Index = m_edges[l].getV1Index();
	  int v2Index = m_edges[l].getV2Index();

	  if ((v1Index == partition[i][j] && v2Index == partition[i][k]) ||
	      (v1Index == partition[i][k] && v2Index == partition[i][j])) {
	    found = true;
	    comVec[i](l) = m_edges[l].getWeight();

	    // qDebug() << i << m_vertices[partition[i][j]].getLabel() << m_vertices[partition[i][k]].getLabel() << m_edges[l].getWeight();
	  }

	  l++;
	}
      }
    }
  }
  
  return comVec;
}

void SocialNetProcessor::displayCommunities(const QVector<int> &comAssign, int minSize, qreal minStrength) const
{
  QVector<QList<int> > partition;

  // retrieve number of communities
  int nCom(0);
  
  for (int i(0); i < comAssign.size(); i++)
    if (comAssign[i] > nCom)
      nCom = comAssign[i];

  partition.resize(nCom + 1);

  // display partition
  for (int i(0); i < comAssign.size(); i++)
    partition[comAssign[i]].push_back(i);

  for (int i(0); i < partition.size(); i++) {

    QList<QPair<qreal, int> > sorted;

    for (int j(0); j < partition[i].size(); j++) {
      qreal strength = m_vertices[partition[i][j]].getWeight();
      sorted.push_back(QPair<qreal, int>(strength, partition[i][j]));
    }

    std::sort(sorted.begin(), sorted.end());
    std::reverse(sorted.begin(), sorted.end());

    if (sorted.size() >= minSize) {
      for (int j(0); j < sorted.size(); j++) {
	QString spkLbl = m_vertices[sorted[j].second].getLabel();
	qreal strength = sorted[j].first;
	
	if (strength >= minStrength)
	  qDebug() << i << spkLbl << strength;
      }

      qDebug();
    }
  }
}

qreal SocialNetProcessor::jaccardIndex(const arma::vec &U, const arma::vec &V) const
{
  qreal jac(-1.0);

  arma::mat EW = U % V;
  arma::uvec N_NULL = arma::find(EW);
  qreal cardInter = N_NULL.n_rows;
  jac = cardInter / U.n_rows;
  // jac = cardInter;

  return jac;
}

qreal SocialNetProcessor::cosineSim(const arma::vec &U, const arma::vec &V) const
{
  qreal cos(0.0);
  
  if (arma::norm(U) > 0 && arma::norm(V) > 0)
    cos = arma::norm_dot(U, V);

  return cos;
}

qreal SocialNetProcessor::l2Dist(const arma::vec &U, const arma::vec &V) const
{
  qreal d(-1.0);
  
  arma::vec UN = arma::normalise(U, 2);
  arma::vec VN = arma::normalise(V, 2);
  
  d = arma::norm(UN - VN, 2);

  return d;
}

void SocialNetProcessor::normalizeWeights()
{
  qreal maxStrength(0.0);
  qreal maxWeight(0.0);

  for (int i(0); i < m_networkViews.size(); i++) {

    for (int j(0); j < m_verticesViews[i].size(); j++) {
      qreal strength = m_verticesViews[i][j].getWeight();
      
      if (strength > maxStrength)
	maxStrength = strength;
    }

    for (int j(0); j < m_edgesViews[i].size(); j++) {
      qreal weight = m_edgesViews[i][j].getWeight();
      
      if (weight > maxWeight)
	maxWeight = weight;
    }
  }

  for (int i(0); i < m_networkViews.size(); i++) {

    qreal normFac(60.0);

    for (int j(0); j < m_verticesViews[i].size(); j++) {
      
      qreal strength = m_verticesViews[i][j].getWeight();
      
      m_verticesViews[i][j].setWeight(strength / maxStrength);

      qreal colorStrength = qPow(strength / maxStrength, 1.0 / normFac);
      m_verticesViews[i][j].setColorWeight(colorStrength);
    }

    for (int j(0); j < m_edgesViews[i].size(); j++) {

      qreal weight = m_edgesViews[i][j].getWeight();
     
      m_edgesViews[i][j].setWeight(weight / maxWeight);
      
      qreal colorWeight = qPow(weight / maxWeight, 1.0 / normFac);
      m_edgesViews[i][j].setColorWeight(colorWeight);
    }
  }
}

void SocialNetProcessor::initGraph(const QMap<QString, QMap<QString, qreal> > &edges)
{
  // retrieving vertices labels
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {
    
    QString fSpk = it1.key();

    if (!m_verticesLabels.contains(fSpk))
      m_verticesLabels.push_back(fSpk);

    QMap<QString, qreal> sSpks = it1.value();

    QMap<QString, qreal>::const_iterator it2 = sSpks.begin();
    
    while (it2 != sSpks.end()) {

      QString sSpk = it2.key();
      
      if (!m_verticesLabels.contains(sSpk))
	m_verticesLabels.push_back(sSpk);
      
      it2++;
    }
    it1++;
  }

  // graph initialization
  if (m_directed)
    igraph_empty(&m_graph, m_verticesLabels.size(), IGRAPH_DIRECTED);
  else
    igraph_empty(&m_graph, m_verticesLabels.size(), IGRAPH_UNDIRECTED);

  // set vertices names
  for (int i(0); i < m_verticesLabels.size(); i++) {
    const char *lbl = m_verticesLabels[i].toLocal8Bit().constData();
    SETVAS(&m_graph, "label", i, lbl);
  }
  
  // set graph edges
  setGraphEdges(edges);
}

void SocialNetProcessor::setGraphEdges(QMap<QString, QMap<QString, qreal> > edges)
{
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  // set edge weights
  it1 = edges.begin();

  while (it1 != edges.end()) {

    QString fSpk = it1.key();
    QMap<QString, qreal> sSpks = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = sSpks.begin();
    while (it2 != sSpks.end()) {

      QString sSpk = it2.key();

      // add edge
      int pFrom = m_verticesLabels.indexOf(fSpk);
      int pTo = m_verticesLabels.indexOf(sSpk);

      igraph_add_edge(&m_graph, pFrom, pTo);

      // set edge weight
      int eid;
      igraph_get_eid(&m_graph, &eid, pFrom, pTo, false, false);

      qreal weight(1.0);
      if (m_weighted)
	weight = it2.value();
      
      SETEAN(&m_graph, "weight", eid, weight);
      
      it2++;
    }
    it1++;
  }
}

void SocialNetProcessor::initGraphWidget()
{
  m_vertices = setVertices();
  m_edges = setEdges();
}

QList<Vertex> SocialNetProcessor::setVertices()
{
  QList<Vertex> vertices;

  // computing node strengths
  QList<qreal> strengths = getNodeStrengths();

  // adding vertices
  for (int i(0); i < strengths.size(); i++) {
    QString label = m_verticesLabels[i];
    QVector3D v = QVector3D(0.0, 0.0, 0.0);
    vertices.push_back(Vertex(v, label));
  }

  return vertices;
}

QList<qreal> SocialNetProcessor::getNodeStrengths()
{
  QList<qreal> strengths;

  // number of vertices and edges
  int nVertices(igraph_vcount(&m_graph));
  int nEdges(igraph_ecount(&m_graph));
  
  // retrieving edge weights
  igraph_vector_t weights;
  igraph_vector_init(&weights, nEdges);
  EANV(&m_graph, "weight", &weights);

  // computing vertex weights
  igraph_vector_t vStrengths;
  igraph_vector_init(&vStrengths, nVertices);
  igraph_strength(&m_graph, &vStrengths, igraph_vss_all(), IGRAPH_ALL, IGRAPH_NO_LOOPS, &weights);

  // pushing normalized degrees
  for (int i(0); i < nVertices; i++)
    strengths.push_back(VECTOR(vStrengths)[i]);

  // memory desallocation
  igraph_vector_destroy(&vStrengths);
  igraph_vector_destroy(&weights);
   
  return strengths;
}

QList<QVector3D> SocialNetProcessor::getLayoutCoord(int nVertices, int nIter, qreal maxdelta, bool use_seed, bool twoDim)
{
  QList<QVector3D> verticesCoord;
  qreal x, y, z;

  // parameters of Fruchterman/Reingold layout
  igraph_vector_t weights;
  igraph_vector_init(&weights, m_edges.size());
  EANV(&m_graph, "weight", &weights);

  igraph_vector_t minx;
  igraph_vector_t maxx;

  igraph_vector_init(&minx, nVertices);
  igraph_vector_init(&maxx, nVertices);

  for (int i(0); i < nVertices; i++) {
    VECTOR(minx)[i] = 500.0;
    VECTOR(maxx)[i] = 500.0;
  }

  if (twoDim) {
    igraph_layout_fruchterman_reingold(&m_graph,
				       &m_coord,
				       use_seed,
				       nIter,
				       maxdelta,
				       IGRAPH_LAYOUT_AUTOGRID,
				       &weights,
				       nullptr,
				       nullptr,
				       nullptr,
				       nullptr);
				       

    // normalizing coordinates
    qreal max(0.0);
    for (int i(0); i < nVertices; i++) {
      x = qFabs(MATRIX(m_coord, i, 0));
      y = qFabs(MATRIX(m_coord, i, 1));
      
      if (x > max)
	max = x;
      if (y > max)
	max = y;
    }

    // pushing coordinates into 3D vector list
    for (int i(0); i < nVertices; i++) {

      x = MATRIX(m_coord, i, 0) / max;
      y = MATRIX(m_coord, i, 1) / max;
      z = 0.0;

      verticesCoord.push_back(QVector3D(x, y, z));
    }
  }

  else {
    igraph_layout_fruchterman_reingold_3d(&m_graph,
					  &m_coord,
					  use_seed,
					  nIter,
					  maxdelta,
					  &weights,
					  nullptr,
					  nullptr,
					  nullptr,
					  nullptr,
					  nullptr,
					  nullptr);
    
    // normalizing coordinates
    qreal max(0.0);
    for (int i(0); i < nVertices; i++) {
      x = qFabs(MATRIX(m_coord, i, 0));
      y = qFabs(MATRIX(m_coord, i, 1));
      z = qFabs(MATRIX(m_coord, i, 2));

      if (x > max)
	max = x;
      if (y > max)
	max = y;
      if (z > max)
	max = z;
    }

    // pushing coordinates into 3D vector list
    for (int i(0); i < nVertices; i++) {

      x = MATRIX(m_coord, i, 0) / max;
      y = MATRIX(m_coord, i, 1) / max;
      z = MATRIX(m_coord, i, 2) / max;

      verticesCoord.push_back(QVector3D(x, y, z));
    }
  }

  igraph_vector_destroy(&weights);
  igraph_vector_destroy(&minx);
  igraph_vector_destroy(&maxx);

  return verticesCoord;
}

QList<Edge> SocialNetProcessor::setEdges()
{
  QList<Edge> edges;

  int nEdges = igraph_ecount(&m_graph);

  for (int i(0); i < nEdges; i++) {
    
    // retrieve indices of adjacent vertices
    int from;
    int to;
    igraph_edge(&m_graph, i, &from, &to);

    // vertices data
    QVector3D v1 = QVector3D(0.0, 0.0, 0.0);
    QVector3D v2 = QVector3D(0.0, 0.0, 0.0);

    // save data
    QPair<qreal, QVector3D> moveToV2(0.0, v2);
    QPair<qreal, QPair<qreal, QVector3D> > v2Data(0.0, moveToV2);

    edges.push_back(Edge(v1, v2Data, from, to));
  }

  return edges;
}

///////////////////////////////////////////////
// update network from view (graph + widget) //
///////////////////////////////////////////////

void SocialNetProcessor::updateGraph(const QMap<QString, QMap<QString, qreal> > &currView)
{
  // delete previous edges
  igraph_es_t es;
  igraph_es_all(&es, IGRAPH_EDGEORDER_ID);
  igraph_delete_edges(&m_graph, es);
  igraph_es_destroy(&es);

  // update edge weight
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = currView.begin();

  while (it1 != currView.end()) {

    QString fSpk = it1.key();
    QMap<QString, qreal> interlocs = it1.value();

    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();
    while (it2 != interlocs.end()) {

      QString sSpk = it2.key();
      qreal weight = it2.value();

      if (weight > 0.0) {

	// edge ends
	int pFrom = m_verticesLabels.indexOf(fSpk);
	int pTo = m_verticesLabels.indexOf(sSpk);
      
	// insert edge
	igraph_add_edge(&m_graph, pFrom, pTo);

	// set edge weight
	int eid;
	igraph_get_eid(&m_graph, &eid, pFrom, pTo, false, false);
	SETEAN(&m_graph, "weight", eid, weight);

	// qDebug() << fSpk << sSpk << weight << pFrom << pTo;
      }
      
      it2++;
    }

    it1++;
  }
}

void SocialNetProcessor::updateGraphWidget(bool layout)
{
  updateVertices(layout);
  updateEdges();
}

void SocialNetProcessor::updateVertices(bool layout)
{
  // updated node strengths
  QList<qreal> strengths = getNodeStrengths();
  for (int i(0); i < strengths.size(); i++) {

    qreal newStrength = strengths[i];
    if (m_vertices[i].getWeight() != newStrength)
      m_vertices[i].setWeight(newStrength);
  }

  if (layout) {
    // possibly update vertices coordinates
    // QList<QVector3D> verticesCoord = getLayoutCoord(m_vertices.size(), 10, 5, true, true);
    // QList<QVector3D> verticesCoord = getLayoutCoord(m_vertices.size(), 10, 20, true, true);
    QList<QVector3D> verticesCoord = getLayoutCoord(m_vertices.size(), 8, 4, true, true);
    for (int i(0); i < verticesCoord.size(); i++)
      m_vertices[i].setV(verticesCoord[i]);
  }
}

void SocialNetProcessor::updateEdges()
{
  int nEdges = igraph_ecount(&m_graph);

  // retrieving edge weights
  igraph_vector_t weights;
  igraph_vector_init(&weights, nEdges);
  EANV(&m_graph, "weight", &weights);

  for (int i(0); i < nEdges; i++) {
    
    // retrieve indices of adjacent vertices
    int from;
    int to;
    igraph_edge(&m_graph, i, &from, &to);

    // edge weight
    qreal edgeWeight = VECTOR(weights)[i];

    // coordinates of adjacent vertices
    QVector3D v1 = m_vertices[from].getV();
    QVector3D v2 = m_vertices[to].getV();

    // z axis along which draw edge cylinder
    QVector3D z(0, 0, 1);

    // second vertex new coordinates
    v2 = v2 - v1;

    // angle between z and v2
    qreal angle = getAngle(z, v2);

    // vector normal n = z ^ v2
    QVector3D n = QVector3D::crossProduct(z, v2);

    // edge length
    qreal length = v2.length();

    // save data
    QPair<qreal, QVector3D> moveToV2(angle, n);
    QPair<qreal, QPair<qreal, QVector3D> > v2Data(length, moveToV2);

    if (edgeWeight > 0.0) {
      m_edges[i].setWeight(edgeWeight);
      m_edges[i].setColorWeight(edgeWeight);
      m_edges[i].setV1(v1);
      m_edges[i].setV2(v2Data);
    }
  }

  // memory desallocation
  igraph_vector_destroy(&weights);
}

void SocialNetProcessor::updateNetView(int iScene)
{
  if (iScene != m_iCurrScene) {

    // update agglomerative graph and corresponding widget
    m_vertices = m_verticesViews[iScene];
    m_edges = m_edgesViews[iScene];

    m_iCurrScene = iScene;
  }
}

void SocialNetProcessor::updateGraph(GraphSource src)
{
  // clear widget graph edges
  m_edges.clear();

  // retrieve graph edges
  QMap<QString, QMap<QString, qreal> > edges;
  
  switch (src) {
  case SocialNetProcessor::CoOccurr:
    edges = getCoOccurrGraph();
    break;
  case SocialNetProcessor::Interact:
    edges = getInteractGraph();
    break;
  }

  // convert graph if necessary
  bool isDirected = igraph_is_directed(&m_graph);
  if (!isDirected && m_directed)
    igraph_to_directed(&m_graph, IGRAPH_TO_DIRECTED_MUTUAL);
  if (isDirected && !m_directed)
    igraph_to_undirected(&m_graph, IGRAPH_TO_UNDIRECTED_COLLAPSE, 0);

  // delete previous edges
  igraph_es_t es;
  igraph_es_all(&es, IGRAPH_EDGEORDER_ID);
  igraph_delete_edges(&m_graph, es);
  igraph_es_destroy(&es);
  
  // set graph edges
  setGraphEdges(edges);

  // set widget node strengths
  QList<qreal> strengths = getNodeStrengths();

  for (int i(0); i < strengths.size(); i++)
    m_vertices[i].setWeight(strengths[i]);

  // set widget edges
  setEdges();
}

///////////////
// modifiers //
///////////////

void SocialNetProcessor::setSceneSpeechSegments(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  m_sceneInteractions.clear();

  m_sceneSpeechSegments = sceneSpeechSegments;
  m_sceneInteractions.resize(m_sceneSpeechSegments.size());
  m_networkViews.resize(m_sceneSpeechSegments.size());
  m_sceneRefs.resize(m_sceneSpeechSegments.size());

  m_verticesViews.resize(m_sceneSpeechSegments.size());
  m_edgesViews.resize(m_sceneSpeechSegments.size());
}

void SocialNetProcessor::setDirected(bool directed)
{
  m_directed = directed;
}

void SocialNetProcessor::setWeighted(bool weighted)
{
  m_weighted = weighted;
}

void SocialNetProcessor::setNbWeight(bool nbWeight)
{
  m_nbWeight = nbWeight;
}

///////////////
// accessors //
///////////////

QVector<QList<Vertex> > SocialNetProcessor::getVerticesViews() const
{
  return m_verticesViews;
}
 
QVector<QList<Edge> > SocialNetProcessor::getEdgesViews() const
{
  return m_edgesViews;
}

QVector<QPair<int, int> > SocialNetProcessor::getSceneRefs() const
{
  return m_sceneRefs;
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::getSceneInteractions() const
{
  return m_sceneInteractions;
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::getNetworkViews() const
{
  return m_networkViews;
}

///////////////////////
// auxiliary methods //
///////////////////////

QMap<QString, QMap<QString, qreal> > SocialNetProcessor::getCoOccurrGraph()
{
  QMap<QString, QMap<QString, qreal> > coOccSpeakers;

  // loop over units
  for (int i(0); i < m_sceneSpeechSegments.size(); i++) {

    QStringList speakers;
    QMap<QString, QStringList> spkHypInterloc;

    for (int j(0); j < m_sceneSpeechSegments[i].size(); j++) {
	
      // current speaker
      QString currSpeaker = m_sceneSpeechSegments[i][j]->getLabel(Segment::Manual);

      // update speaker interactions
      if (!speakers.contains(currSpeaker)) {
	speakers.push_back(currSpeaker);
	
	// current interlocutors
	QStringList hypInterLoc = m_sceneSpeechSegments[i][j]->getInterLocs(SpkInteractDialog::CoOccurr);
	
	// loop over hypothesized interlocutors
	for (int k(0); k < hypInterLoc.size(); k++) {
	  if (!spkHypInterloc[currSpeaker].contains(hypInterLoc[k])) {

	    QString fSpk = currSpeaker;
	    QString sSpk = hypInterLoc[k];

	    if (!m_directed) {
	      fSpk = (currSpeaker < hypInterLoc[k]) ? currSpeaker : hypInterLoc[k];
	      sSpk = (currSpeaker > hypInterLoc[k]) ? currSpeaker : hypInterLoc[k];
	    }

	    coOccSpeakers[fSpk][sSpk]++;
	    m_sceneInteractions[i][fSpk][sSpk]++;
	  }
	}
      }
    }
  }

  return coOccSpeakers;
}

QMap<QString, QMap<QString, qreal> > SocialNetProcessor::getInteractGraph()
{
  QMap<QString, QMap<QString, qreal> > interactSpeakers;

  // loop over units
  for (int i(0); i < m_sceneSpeechSegments.size(); i++)
    for (int j(0); j < m_sceneSpeechSegments[i].size(); j++) {

      QString currSpk = m_sceneSpeechSegments[i][j]->getLabel(Segment::Manual);
      qreal duration = (m_sceneSpeechSegments[i][j]->getEnd() - m_sceneSpeechSegments[i][j]->getPosition()) / 1000.0;
      
      QStringList hypInterLoc = m_sceneSpeechSegments[i][j]->getInterLocs(SpkInteractDialog::Sequential);
      
      // update interactions
      for (int k(0); k < hypInterLoc.size(); k++) {

	QString fSpk = currSpk;
	QString sSpk = hypInterLoc[k];

	if (!m_directed) {
	  fSpk = (currSpk < hypInterLoc[k]) ? currSpk : hypInterLoc[k];
	  sSpk = (currSpk > hypInterLoc[k]) ? currSpk : hypInterLoc[k];
	}

	qreal weight(0.0);

	if (m_nbWeight) {
	  weight = 1.0 / hypInterLoc.size();
	  interactSpeakers[fSpk][sSpk] += weight;
	  m_sceneInteractions[i][fSpk][sSpk] += weight;
	}
	
	else {
	  weight = duration / hypInterLoc.size();
	  interactSpeakers[fSpk][sSpk] += weight;
	  m_sceneInteractions[i][fSpk][sSpk] += weight;
	}
      }
    }

  return interactSpeakers;
}

qreal SocialNetProcessor::getAngle(const QVector3D &v1, const QVector3D &v2)
{
  qreal angle = qAtan2(QVector3D::crossProduct(v1, v2).length(), QVector3D::dotProduct(v1, v2));

  return qRadiansToDegrees(angle);
}

//////////
// misc //
//////////

qreal SocialNetProcessor::communityDetection(QVector<QList<QPair<qreal, QString> > > &partition)
{
  qreal score(0.0);

  // number of vertices and edges
  int nVertices(igraph_vcount(&m_graph));
  int nEdges(igraph_ecount(&m_graph));
  
  // retrieving edge weights
  igraph_vector_t edgeWeights;
  igraph_vector_init(&edgeWeights, nEdges);
  EANV(&m_graph, "weight", &edgeWeights);

  // retrieving node strengths
  igraph_vector_t strengths;
  igraph_vector_init(&strengths, nVertices);
  igraph_strength(&m_graph, &strengths, igraph_vss_all(), IGRAPH_ALL, IGRAPH_NO_LOOPS, &edgeWeights);

  igraph_vector_t modularity;
  igraph_vector_t membership;
  igraph_matrix_t memberships;
  igraph_real_t codelength(0.0);
  int nb_trials(10);

  igraph_vector_init(&modularity, 0);
  igraph_vector_init(&membership, 0);
  igraph_matrix_init(&memberships, 0, 0);

  // igraph_community_multilevel(&m_graph, &edgeWeights, &membership, &memberships, &modularity);
  igraph_community_infomap(&m_graph, &edgeWeights, &strengths, nb_trials, &membership, &codelength);
  // if (!igraph_vector_empty(&modularity))
  // score = igraph_vector_max(&modularity);
  score = codelength;

  // number of modularity classes
  int nbClasses(0);
  for (int i(0); i < nVertices; i++) {
    int modularityClass = VECTOR(membership)[i];
    if (modularityClass > nbClasses)
      nbClasses = modularityClass;
  }

  partition.resize(++nbClasses);

  for (int i(0); i < nVertices; i++) {
    
    QString label = igraph_cattribute_VAS(&m_graph, "label", i);
    qreal strength = VECTOR(strengths)[i];
    int modularityClass = VECTOR(membership)[i];

    // qDebug() << label << strength << modularityClass << nbClasses;

    partition[modularityClass].push_back(QPair<qreal, QString>(strength, label));
  }
  
  // remove communities containing a single element
  for (int i(partition.size() - 1); i >= 0; i--)
    if (partition[i].size() < 2)
      partition.remove(i);

  igraph_vector_destroy(&modularity);
  igraph_vector_destroy(&membership);
  igraph_matrix_destroy(&memberships);
  igraph_vector_destroy(&edgeWeights);
  igraph_vector_destroy(&strengths);

  return score;
}

QVector<int> SocialNetProcessor::communityDetection_bis()
{
  QVector<int> comAssign;

  // number of vertices and edges
  int nVertices(igraph_vcount(&m_graph));
  int nEdges(igraph_ecount(&m_graph));
  
  comAssign.resize(nVertices);

  // retrieving edge weights
  igraph_vector_t edgeWeights;
  igraph_vector_init(&edgeWeights, nEdges);
  EANV(&m_graph, "weight", &edgeWeights);

  // retrieving node strengths
  igraph_vector_t strengths;
  igraph_vector_init(&strengths, nVertices);
  igraph_strength(&m_graph, &strengths, igraph_vss_all(), IGRAPH_ALL, IGRAPH_NO_LOOPS, &edgeWeights);

  igraph_vector_t modularity;
  igraph_vector_t membership;
  igraph_matrix_t memberships;
  igraph_real_t codelength(0.0);
  int nb_trials(10);

  igraph_vector_init(&modularity, 0);
  igraph_vector_init(&membership, 0);
  igraph_matrix_init(&memberships, 0, 0);

  igraph_community_multilevel(&m_graph, &edgeWeights, &membership, &memberships, &modularity);
  // igraph_community_infomap(&m_graph, &edgeWeights, &strengths, nb_trials, &membership, &codelength);

  // assign communities
  for (int i(0); i < nVertices; i++)
    comAssign[i] = VECTOR(membership)[i];

  igraph_vector_destroy(&modularity);
  igraph_vector_destroy(&membership);
  igraph_matrix_destroy(&memberships);
  igraph_vector_destroy(&edgeWeights);
  igraph_vector_destroy(&strengths);

  return comAssign;
}

QVector<int> SocialNetProcessor::communityDetection_ter()
{
  QVector<int> comAssign;

  // number of vertices and edges
  int nVertices(igraph_vcount(&m_graph));
  int nEdges(igraph_ecount(&m_graph));
  
  comAssign.resize(nVertices);

  // retrieving edge weights
  igraph_vector_t edgeWeights;
  igraph_vector_init(&edgeWeights, nEdges);
  EANV(&m_graph, "weight", &edgeWeights);

  igraph_vector_t modularity;
  igraph_vector_t membership;
  igraph_matrix_t memberships;

  igraph_vector_init(&modularity, 0);
  igraph_vector_init(&membership, 0);
  igraph_matrix_init(&memberships, 0, 0);

  igraph_community_multilevel(&m_graph, &edgeWeights, &membership, &memberships, &modularity);

  // assign communities
  for (int i(0); i < nVertices; i++)
    comAssign[i] = VECTOR(membership)[i];

  igraph_vector_destroy(&modularity);
  igraph_vector_destroy(&membership);
  igraph_matrix_destroy(&memberships);
  igraph_vector_destroy(&edgeWeights);

  return comAssign;
}

void SocialNetProcessor::exportViewToFile(int i, const QString &baseName, const QString &method)
{
  // set graph to current view
  updateGraph(m_networkViews[i]);

  // retrieve seasons, episode and scene index
  QString sceneRef;

  if (!m_sceneSpeechSegments[i].isEmpty()) {

    Episode *currEpisode = static_cast<Episode *>(m_sceneSpeechSegments[i].first()->parent());
    Season *currSeason = static_cast<Season *>(currEpisode->parent());

    QString seasonNbr = QString::number(currSeason->getNumber());
    if (seasonNbr.length() == 1)
      seasonNbr = "0" + seasonNbr;
  
    QString episodeNbr = QString::number(currEpisode->getNumber());
    if (episodeNbr.length() == 1)
      episodeNbr = "0" + episodeNbr;

    QString sceneNbr = QString::number(i);
    int nChar = sceneNbr.length();
    QString pref;

    switch (nChar) {
    case 1:
      pref = "00";
      break;
    case 2:
      pref = "0";
      break;
    case 3:
      pref = "";
      break;
    default:
      break;
    }

    sceneNbr = pref + sceneNbr;
    sceneRef = "S" + seasonNbr + "E" + episodeNbr + "_" + sceneNbr;

    if (sceneRef != "") {
      QString fName = "/home/xbost/Outils/tv_series_proc_tool/tools/sna/graph_files/" + baseName + "_" + method + "/" + baseName + "_" + sceneRef + ".graphml";
      qDebug() << fName;
      exportGraphToGraphmlFile(m_networkViews[i], fName);
    }

    m_sceneRefs[i] = QPair<int, int>(currSeason->getNumber(), currEpisode->getNumber());
  }
}

void SocialNetProcessor::exportGraphToGraphmlFile(const QMap<QString, QMap<QString, qreal> > &edges, const QString &fName)
{
  QFile graphmlFile(fName);

  if (!graphmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QTextStream graphmlOut(&graphmlFile);

  QString xmlLine = 
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  
  QString preambule = 
    "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"\n\txmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n\txsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns\n\thttp://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">";
  
  QString keys = 
    "\t<key id=\"label\" for=\"node\" attr.name=\"label\" attr.type=\"string\"/>\n\t<key id=\"weight\" for=\"edge\" attr.name=\"weight\" attr.type=\"double\"/>";
  
  QString graph = 
    "\t<graph id=\"G\" edgedefault=\"undirected\">";

  graphmlOut << xmlLine << endl;
  graphmlOut << preambule << endl;
  graphmlOut << keys << endl;
  graphmlOut << graph << endl;
  
  for (int i(0); i < m_verticesLabels.size(); i++) {
    QString node = "\t\t<node id=\"n" + QString::number(i) + "\">";
    node += "\n\t\t\t<data key=\"label\">";
    node += m_verticesLabels[i];
    node += "</data>\n\t\t</node>";

    graphmlOut << node << endl;
  }

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {
    
    QString fSpk = it1.key();
    QMap<QString, qreal> sSpks = it1.value();

    QMap<QString, qreal>::const_iterator it2 = sSpks.begin();
    
    while (it2 != sSpks.end()) {

      QString sSpk = it2.key();
      qreal weight = it2.value();

      int i = m_verticesLabels.indexOf(fSpk);
      int j = m_verticesLabels.indexOf(sSpk);

      QString edge = "\t\t<edge source=\"n" + QString::number(i);
      edge += "\" target=\"n" + QString::number(j) + "\">";
      edge += "\n\t\t\t<data key=\"weight\">";
      // edge += QString::number(weight, 'f', 4);
      edge += QString::number(weight);
      edge += "</data>\n\t\t</edge>";

      if (weight > 0.0)
	graphmlOut << edge << endl;

      it2++;
    }
    it1++;
  }

  graphmlOut << "\t</graph>" << endl;
  graphmlOut << "</graphml>" << endl;
}

void SocialNetProcessor::exportGraphToPajekFile(const QMap<QString, QMap<QString, qreal> > &edges, const QString &fName)
{
  QFile netFile(fName);

  if (!netFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QTextStream netOut(&netFile);
  
  netOut << "*Vertices " << m_verticesLabels.size() << endl;

  for (int i(0); i < m_verticesLabels.size(); i++)
    netOut << i + 1 << " \"" << m_verticesLabels[i] << "\"" << endl;

  // write out edges weights
  netOut << "*Edges " << edges.size() << endl;

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {
    
    QString fSpk = it1.key();
    QMap<QString, qreal> sSpks = it1.value();

    QMap<QString, qreal>::const_iterator it2 = sSpks.begin();
    
    while (it2 != sSpks.end()) {

      QString sSpk = it2.key();
      qreal weight = it2.value();

      int i = m_verticesLabels.indexOf(fSpk) + 1;
      int j = m_verticesLabels.indexOf(sSpk) + 1;

      netOut << i << " " << j << " " << QString::number(weight, 'f', 4) << endl;
      
      it2++;
    }
    it1++;
  }
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::buildNetworkSnapshots_bis()
{
  QVector<QMap<QString, QMap<QString, qreal> > > networkSnapshotsFromPast = buildNetworkSnapshotsFromPast();
  QVector<QMap<QString, QMap<QString, qreal> > > networkSnapshotsFromFuture = buildNetworkSnapshotsFromFuture();

  QVector<QMap<QString, QMap<QString, qreal> > > networkSnapshots = mergeNetworkSnapshots(networkSnapshotsFromPast, networkSnapshotsFromFuture);

  /*
  for (int i(0); i < networkSnapshots.size(); i++) {

    // loop over interactions
    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = networkSnapshots[i].begin();

    while (it1 != networkSnapshots[i].end()) {

      QString fSpk = it1.key();
      QMap<QString, qreal> interloc = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interloc.begin();

      while (it2 != interloc.end()) {

	QString sSpk = it2.key();
	qreal weight = it2.value();

	qDebug() << i << fSpk << sSpk << weight;
	  
	it2++;
      }
      it1++;
    }
  }
  */

  return networkSnapshots;
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::mergeNetworkSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &networkSnapshotsFromPast, const QVector<QMap<QString, QMap<QString, qreal> > > &networkSnapshotsFromFuture)
{
  QVector<QMap<QString, QMap<QString, qreal> > > networkSnapshots(networkSnapshotsFromPast.size());

  for (int i(0); i < networkSnapshots.size() - 1; i++) {

    QMap<QString, QMap<QString, qreal> > snapshot;

    QMap<QString, QMap<QString, qreal> > snapshotFromPast = networkSnapshotsFromPast[i];
    QMap<QString, QMap<QString, qreal> > snapshotFromFuture = networkSnapshotsFromFuture[i+1];

    // loop over interactions built from the past
    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = snapshotFromPast.begin();

    while (it1 != snapshotFromPast.end()) {

      QString fSpk = it1.key();
      QMap<QString, qreal> interloc = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interloc.begin();

      while (it2 != interloc.end()) {

	QString sSpk = it2.key();
	qreal weight = it2.value();

	// relation activated both in the past and the future
	if (snapshotFromFuture.contains(fSpk) && snapshotFromFuture[fSpk].contains(sSpk))
	  snapshot[fSpk][sSpk] = (weight > snapshotFromFuture[fSpk][sSpk]) ? sigmoid(weight) : sigmoid(snapshotFromFuture[fSpk][sSpk]);
	// snapshot[fSpk][sSpk] = (weight > snapshotFromFuture[fSpk][sSpk]) ? weight : snapshotFromFuture[fSpk][sSpk];

	// relation activated in the past but not in the future:
	
	// exactly one speaker interacting in the future
	else if (interactingSpeaker(fSpk, snapshotFromFuture) || interactingSpeaker(sSpk, snapshotFromFuture))
	  snapshot[fSpk][sSpk] = sigmoid(weight);
	// snapshot[fSpk][sSpk] = weight;

	// neither speaker interacting in the future
	else
	  snapshot[fSpk][sSpk] = 0.0;
	  
	it2++;
      }
      it1++;
    }

    // loop over interactions built from the future
    it1 = snapshotFromFuture.begin();

    while (it1 != snapshotFromFuture.end()) {

      QString fSpk = it1.key();
      QMap<QString, qreal> interloc = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interloc.begin();

      while (it2 != interloc.end()) {

	QString sSpk = it2.key();
	qreal weight = it2.value();
	
	// relation activated in the future but not in the past:
	if (!snapshotFromPast.contains(fSpk) || !snapshotFromPast[fSpk].contains(sSpk)) {

	  // exactly one speaker interacting in the past
	  if (interactingSpeaker(fSpk, snapshotFromPast) || interactingSpeaker(sSpk, snapshotFromPast))
	    snapshot[fSpk][sSpk] = sigmoid(weight);
	    // snapshot[fSpk][sSpk] = weight;
	
	  // neither speaker interacting in the past
	  else
	    snapshot[fSpk][sSpk] = 0.0;
	}

	it2++;
      }
      it1++;
    }
    
    networkSnapshots[i] = snapshot;
  }

  return networkSnapshots;
}

qreal SocialNetProcessor::sigmoid(qreal x, qreal lambda)
{
  return 1.0 / (1 + qExp(-lambda * x));
}

bool SocialNetProcessor::interactingSpeaker(const QString &spk, const QMap<QString, QMap<QString, qreal> > &network)
{
  if (network.contains(spk))
    return true;
  else {
    QMap<QString, QMap<QString, qreal> >::const_iterator it = network.begin();

    while (it != network.end()) {
      
      QMap<QString, qreal> interloc = it.value();
      
      if (interloc.contains(spk))
	return true;

      it++;
    }
  }

  return false;
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::buildNetworkSnapshotsFromPast()
{
  QVector<QMap<QString, QMap<QString, qreal> > > networkSnapshotsFromPast(m_sceneInteractions.size());

  for (int i(0); i < m_sceneInteractions.size(); i++) {
    
    QMap<QString, QMap<QString, qreal> > networkSnapshot;
    QMap<QString, QMap<QString, qreal> > currSceneInteractions = m_sceneInteractions[i];

    // increment interaction weights
    if (i > 0) {

      networkSnapshot = networkSnapshotsFromPast[i-1];

      QMap<QString, QMap<QString, qreal> >::const_iterator it1 = currSceneInteractions.begin();

      while (it1 != currSceneInteractions.end()) {

	QString fSpk = it1.key();
	QMap<QString, qreal> interloc = it1.value();

	QMap<QString, qreal>::const_iterator it2 = interloc.begin();

	while (it2 != interloc.end()) {

	  QString sSpk = it2.key();
	  qreal weight = it2.value();

	  // qDebug() << endl << i << fSpk << sSpk << weight;

	  updateSnapshot(networkSnapshot, fSpk, sSpk, weight);

	  it2++;
	}

	it1++;
      }
    }

    // set state of current interactions
    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = currSceneInteractions.begin();

    while (it1 != currSceneInteractions.end()) {

      QString fSpk = it1.key();
      QMap<QString, qreal> interloc = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interloc.begin();

      while (it2 != interloc.end()) {

	QString sSpk = it2.key();
	qreal weight = it2.value();

	networkSnapshot[fSpk][sSpk] = weight;

	it2++;
      }

      it1++;
    }

    networkSnapshotsFromPast[i] = networkSnapshot;
  }

  return networkSnapshotsFromPast;
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::buildNetworkSnapshotsFromFuture()
{
  QVector<QMap<QString, QMap<QString, qreal> > > networkSnapshotsFromFuture(m_sceneInteractions.size());

  for (int i(m_sceneInteractions.size() - 1); i >= 0 ; i--) {
    
    QMap<QString, QMap<QString, qreal> > networkSnapshot;
    QMap<QString, QMap<QString, qreal> > currSceneInteractions = m_sceneInteractions[i];

    // decrement interaction weights
    if (i < m_sceneInteractions.size() - 1) {

      networkSnapshot = networkSnapshotsFromFuture[i+1];

      QMap<QString, QMap<QString, qreal> >::const_iterator it1 = currSceneInteractions.begin();

      while (it1 != currSceneInteractions.end()) {

	QString fSpk = it1.key();
	QMap<QString, qreal> interloc = it1.value();

	QMap<QString, qreal>::const_iterator it2 = interloc.begin();

	while (it2 != interloc.end()) {

	  QString sSpk = it2.key();
	  qreal weight = it2.value();

	  // qDebug() << endl << i << fSpk << sSpk << weight;

	  updateSnapshot(networkSnapshot, fSpk, sSpk, weight);

	  it2++;
	}

	it1++;
      }
    }

    // set state of current interactions
    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = currSceneInteractions.begin();

    while (it1 != currSceneInteractions.end()) {

      QString fSpk = it1.key();
      QMap<QString, qreal> interloc = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interloc.begin();

      while (it2 != interloc.end()) {

	QString sSpk = it2.key();
	qreal weight = it2.value();

	networkSnapshot[fSpk][sSpk] = weight;

	// qDebug() << endl << i << fSpk << sSpk << weight;

	it2++;
      }

      it1++;
    }
    
    networkSnapshotsFromFuture[i] = networkSnapshot;
  }

  return networkSnapshotsFromFuture;
}

void SocialNetProcessor::updateSnapshot(QMap<QString, QMap<QString, qreal> > &snapshot, const QString &fSpeaker, const QString &sSpeaker, qreal weight)
{
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = snapshot.begin();

  while (it1 != snapshot.end()) {

    QString fSpk = it1.key();
    QMap<QString, qreal> interloc = it1.value();

    QMap<QString, qreal>::const_iterator it2 = interloc.begin();

    while (it2 != interloc.end()) {

      QString sSpk = it2.key();

      if (fSpeaker == fSpk || fSpeaker == sSpk || sSpeaker == sSpk || sSpeaker == fSpk) {
	snapshot[fSpk][sSpk] -= weight;
	
	// qDebug() << fSpk << sSpk << snapshot[fSpk][sSpk];
      }
      it2++;
    }
    it1++;
  }
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::buildNetworkSnapshots()
{
  QVector<QMap<QString, QMap<QString, qreal> > > networkSnapshots(m_sceneInteractions.size());
  
  // scene occurences of speakers and interactions
  QMap<QString, QList<QPair<int, qreal> > > speakerOcc;
  QMap<QString, QMap<QString, QList<QPair<int, qreal> > > > interOcc;

  speakerOcc = getSpeakerOcc(m_sceneInteractions);
  interOcc = getInterOcc(m_sceneInteractions);

  // loop over interactions for building network views
  QMap<QString, QMap<QString, QList<QPair<int, qreal> > > >::const_iterator it1 = interOcc.begin();

  while (it1 != interOcc.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, QList<QPair<int, qreal> > > interLocs = it1.value();

    QMap<QString, QList<QPair<int, qreal> > >::const_iterator it2 = interLocs.begin();

    while (it2 != interLocs.end()) {

      QString sSpeaker = it2.key();
      QList<QPair<int, qreal> > scenesInter = it2.value();

      // weight interaction in each scene
      QVector<qreal> snapshotInter = getSceneWeights(scenesInter, speakerOcc[fSpeaker], speakerOcc[sSpeaker], m_sceneInteractions.size());

      // update snapshots
      for (int i(0); i < snapshotInter.size(); i++)
	networkSnapshots[i][fSpeaker][sSpeaker] = snapshotInter[i];

      it2++;
    }

    it1++;
  }

  return networkSnapshots;
}

QVector<QMap<QString, QMap<QString, qreal> > > SocialNetProcessor::buildCumNetworks(int timeSlice)
{
  QVector<QMap<QString, QMap<QString, qreal> > > cumNetworks;
  cumNetworks.resize(m_sceneInteractions.size() / timeSlice);

  // qDebug() << m_sceneInteractions.size() << timeSlice;

  for (int i(0); i < cumNetworks.size(); i++) {
    
    int first = i * timeSlice;
    int last = timeSlice * (i + 1);

    // qDebug() << first << "->" << last;

    for (int j(first); j < last; j++) {

      QMap<QString, QMap<QString, qreal> >::const_iterator it1 = m_sceneInteractions[j].begin();

      while (it1 != m_sceneInteractions[j].end()) {

	QString fSpeaker = it1.key();
	QMap<QString, qreal> interlocs = it1.value();

	QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

	while (it2 != interlocs.end()) {
	
	  QString sSpeaker = it2.key();
	  qreal duration = it2.value();

	  cumNetworks[i][fSpeaker][sSpeaker] += duration;
	
	  // qDebug() << fSpeaker << sSpeaker << duration;

	  it2++;
	}

	it1++;
      }
    }
  }

  QVector<QMap<QString, QMap<QString, qreal> > > expandedNet;
  expandedNet.resize(cumNetworks.size() * timeSlice);

  // expand the network to intermediate scenes
  for (int i(0); i < cumNetworks.size(); i++) {

    int first = i * timeSlice;
    int last = timeSlice * (i + 1);
    
    for (int j(first); j < last; j++)
      expandedNet[j] = cumNetworks[i];
  }

  // qDebug() << m_sceneInteractions.size() << expandedNet.size();

  return expandedNet;
}

QVector<qreal> SocialNetProcessor::getSceneWeights(const QList<QPair<int, qreal> > &interOcc, const QList<QPair<int, qreal> > &fSpeakerOcc, const QList<QPair<int, qreal> > &sSpeakerOcc, int n)
{
  QVector<qreal> sceneWeights(n, -100000);
  QList<int> fSpkOccIndices;
  QList<int> sSpkOccIndices;
  
  // retrieve speaker occurrence indices
  for (int i(0); i < fSpeakerOcc.size(); i++)
    fSpkOccIndices.push_back(fSpeakerOcc[i].first);
  
  for (int i(0); i < sSpeakerOcc.size(); i++)
    sSpkOccIndices.push_back(sSpeakerOcc[i].first);

  // qDebug() << interOcc;
  // qDebug() << fSpeakerOcc;
  // qDebug() << sSpeakerOcc;

  if (interOcc.size() == 1)
    sceneWeights[interOcc[0].first] = interOcc[0].second;

  else {
    // loop over scenes where interactions take place
    for (int i(0); i < interOcc.size() - 1; i++) {

      // index of previous and last interactions
      int iPrev = interOcc[i].first;
      int iNext = interOcc[i+1].first;

      // qDebug() << "(" << iPrev << "," << iNext << ")";

      // retrieve intermediate interactions
      int iPrevFSpk = fSpkOccIndices.indexOf(iPrev);
      int iNextFSpk = fSpkOccIndices.indexOf(iNext);

      int iPrevSSpk = sSpkOccIndices.indexOf(iPrev);
      int iNextSSpk = sSpkOccIndices.indexOf(iNext);

      QList<int> fSpkMed = fSpkOccIndices.mid(iPrevFSpk, iNextFSpk - iPrevFSpk + 1);
      QList<int> sSpkMed = sSpkOccIndices.mid(iPrevSSpk, iNextSSpk - iPrevSSpk + 1);

      int fSpkMax = (fSpkMed[fSpkMed.size() - 2]);
      int sSpkMax = (sSpkMed[sSpkMed.size() - 2]);
    
      int max = (fSpkMax > sSpkMax) ? fSpkMax : sSpkMax;

      qreal durFromPrev = interOcc[i].second;
      qreal durToNext = interOcc[i+1].second;

      sceneWeights[iPrev] = durFromPrev;
      sceneWeights[iNext] = durToNext;

      for (int j(iNext - 1); j > iPrev; j--)
	if (fSpkMed.contains(j))
	  durToNext -= fSpeakerOcc[fSpkOccIndices.indexOf(j)].second;
	else if (sSpkMed.contains(j))
	  durToNext -= sSpeakerOcc[sSpkOccIndices.indexOf(j)].second;

      // qDebug() << iPrev << sceneWeights[iPrev];

      for (int j(iPrev + 1); j < iNext; j++) {
      
	if (fSpkMed.contains(j))
	  durFromPrev -= fSpeakerOcc[fSpkOccIndices.indexOf(j)].second;
      
	else if (sSpkMed.contains(j))
	  durFromPrev -= sSpeakerOcc[sSpkOccIndices.indexOf(j)].second;
	
	sceneWeights[j] = (durFromPrev > durToNext) ? durFromPrev : durToNext;
	// qDebug() << j << sceneWeights[j];

	if (j < max) {
	  if (fSpkMed.contains(j))
	    durToNext += fSpeakerOcc[fSpkOccIndices.indexOf(j)].second;
      
	  else if (sSpkMed.contains(j))
	    durToNext += sSpeakerOcc[sSpkOccIndices.indexOf(j)].second;
	}
      }
    }

    // qDebug() << iNext << sceneWeights[iNext];
  }

  // process scenes before first occurrence of interaction
  QList<int> fSpkPrev;
  QList<int> sSpkPrev;

  qreal prevDuration(0.0);
  for (int i(0); i < interOcc.first().first; i++) {

    if (fSpkOccIndices.contains(i)) {
      fSpkPrev.push_back(i);
      prevDuration += fSpeakerOcc[fSpkOccIndices.indexOf(i)].second;
    }

    if (sSpkOccIndices.contains(i)) {
      sSpkPrev.push_back(i);
      prevDuration += sSpeakerOcc[sSpkOccIndices.indexOf(i)].second;
    }
  }
  
  if (prevDuration > 0) {
    
    int firstSpkOcc;
    int lastSpkOcc;

    if (!fSpkPrev.isEmpty() && !sSpkPrev.isEmpty()) {
      firstSpkOcc = (fSpkPrev.first() < sSpkPrev.first()) ? fSpkPrev.first() : sSpkPrev.first();
      lastSpkOcc = (fSpkPrev.last() > sSpkPrev.last()) ? fSpkPrev.last() : sSpkPrev.last();
    }
    else if (!fSpkPrev.isEmpty()) {
      firstSpkOcc = fSpkPrev.first();
      lastSpkOcc = fSpkPrev.last();
    }
    else {
      firstSpkOcc = sSpkPrev.first();
      lastSpkOcc = sSpkPrev.last();
    }

    qreal fromFirstInter = interOcc.first().second - prevDuration;

    for (int i(firstSpkOcc); i < interOcc.first().first; i++) {

      sceneWeights[i] = fromFirstInter;

      if (i < lastSpkOcc) {
	if (fSpkOccIndices.contains(i))
	  fromFirstInter += fSpeakerOcc[fSpkOccIndices.indexOf(i)].second;
	else if (sSpkOccIndices.contains(i))
	  fromFirstInter += sSpeakerOcc[sSpkOccIndices.indexOf(i)].second;
      }
    }
  }

  // process scenes after last occurrence of interaction
  QList<int> fSpkFollow;
  QList<int> sSpkFollow;

  for (int i(interOcc.last().first + 1); i < n; i++) {
    
    if (fSpkOccIndices.contains(i))
      fSpkFollow.push_back(i);

    if (sSpkOccIndices.contains(i))
      sSpkFollow.push_back(i);
  }

  int lastSpkOcc;

  if (!fSpkFollow.isEmpty() || !sSpkFollow.isEmpty()) {

    if (!fSpkFollow.isEmpty() && !sSpkFollow.isEmpty())
      lastSpkOcc = (fSpkFollow.last() > sSpkFollow.last()) ? fSpkFollow.last() : sSpkFollow.last();
    else if (!fSpkFollow.isEmpty())
      lastSpkOcc = fSpkFollow.last();
    else
      lastSpkOcc = sSpkFollow.last();
    
    qreal fromLastInter = interOcc.last().second;

    for (int i(interOcc.last().first + 1); i <= lastSpkOcc; i++) {

      if (fSpkOccIndices.contains(i))
	fromLastInter -= fSpeakerOcc[fSpkOccIndices.indexOf(i)].second;
      else if (sSpkOccIndices.contains(i))
	fromLastInter -= sSpeakerOcc[sSpkOccIndices.indexOf(i)].second;
  
      sceneWeights[i] = fromLastInter;
    }
  }

  sceneWeights = weightEdges(sceneWeights, 0.01);

  return sceneWeights;
}

QVector<qreal> SocialNetProcessor::weightEdges(const QVector<qreal> &sceneWeights, qreal lambda) const
{
  QVector<qreal> edgeWeights(sceneWeights.size());

  for (int i(0); i < sceneWeights.size(); i++)
    edgeWeights[i] = 1.0 / (1 + qExp(-lambda * sceneWeights[i]));

  return edgeWeights;
}

QMap<QString, QList<QPair<int, qreal> > > SocialNetProcessor::getSpeakerOcc(const QVector<QMap<QString, QMap<QString, qreal> > > &inter)
{
  QMap<QString, QList<QPair<int, qreal> > > speakerOcc;

  // get speakers/nodes list
  for (int i(0); i < inter.size(); i++) {

    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = inter[i].begin();

    QMap<QString, qreal> interDuration;

    while (it1 != inter[i].end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interlocs = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

      while (it2 != interlocs.end()) {
      
	QString sSpeaker = it2.key();
	qreal duration = it2.value();

	interDuration[fSpeaker] += duration;
	interDuration[sSpeaker] += duration;
	
	it2++;
      }

      it1++;
    }

    QMap<QString, qreal>::const_iterator it3 = interDuration.begin();

    while (it3 != interDuration.end()) {
      
      QString speaker = it3.key();
      qreal duration = it3.value();

      speakerOcc[speaker].push_back(QPair<int, qreal>(i, duration));

      it3++;
    }
  }

  return speakerOcc;
}

QMap<QString, QMap<QString, QList<QPair<int, qreal> > > > SocialNetProcessor::getInterOcc(const QVector<QMap<QString, QMap<QString, qreal> > > &inter)
{
  QMap<QString, QMap<QString, QList<QPair<int, qreal> > > > interOcc;

  // loop over narrative units
  for (int i(0); i < inter.size(); i++) {

    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = inter[i].begin();

    while (it1 != inter[i].end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interlocs = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

      while (it2 != interlocs.end()) {
	
	QString sSpeaker = it2.key();
	qreal duration = it2.value();
	
	interOcc[fSpeaker][sSpeaker].push_back(QPair<int, qreal>(i, duration));
	
	it2++;
      }

      it1++;
    }
  }

  return interOcc;
}

void SocialNetProcessor::displaySceneSpeakers() const
{
  for (int i(0); i < m_sceneInteractions.size(); i++) {
    
    QStringList speakers = getInteractingSpeakers(m_sceneInteractions[i]);

    qDebug() << i << speakers;
  }
}

QStringList SocialNetProcessor::getInteractingSpeakers(const QMap<QString, QMap<QString, qreal> > &interactions) const
{
  QStringList speakers;

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = interactions.begin();

    while (it1 != interactions.end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interlocs = it1.value();

      if (!speakers.contains(fSpeaker))
	speakers.push_back(fSpeaker);

      QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

      while (it2 != interlocs.end()) {
	
	QString sSpeaker = it2.key();

	if (!speakers.contains(sSpeaker))
	  speakers.push_back(sSpeaker);

	it2++;
      }
      it1++;
    }
    
    return speakers;
}

void SocialNetProcessor::monitorPlot()
{
  /*
  arma::mat F;
  F.zeros(3, m_sceneSpeechSegments.size());
  
  for (int i(0); i < m_sceneSpeechSegments.size(); i++) {
    QVector<int> I = {0, 0, 0};
    for (int j(0); j < m_sceneSpeechSegments[i].size(); j++) {

      QString spk = m_sceneSpeechSegments[i][j]->getLabel(Segment::Manual);

      if (spk == "Tyrion Lannister")
	I[0] = 4;
      else if (spk == "Daenerys Targaryen")
	I[1] = 3;
      else if (spk == "Jon Snow")
	I[2] = 2;
    }
    for (int j(0); j < I.size(); j++)
      F(j, i) = I[j];
  }

  F.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/F.dat", arma::raw_ascii);
  */
  
  m_networkViews = buildNetworkSnapshots();
  
  /*
  arma::mat W;
  W.zeros(3, m_networkViews.size());
  W.row(0) = (monitorRelationWeight("Francis Underwood", "Claire Underwood", m_networkViews)).t();
  W.row(1) = (monitorRelationWeight("Francis Underwood", "Martin Spinella", m_networkViews)).t();
  W.row(2) = (monitorRelationWeight("Lucas Goodwin", "Gavin Orsay", m_networkViews)).t();
  W.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/W.dat", arma::raw_ascii);
  */

  arma::mat S;
  S.zeros(2, m_networkViews.size());

  /*
  S.row(0) = (monitorSpkStrength("Walter White", m_networkViews)).t();
  S.row(1) = (monitorSpkStrength("Tuco Salamanca", m_networkViews)).t();
  S.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/S.dat", arma::raw_ascii);
  */

  S.row(0) = monitorSpkStrength("Daenerys Targaryen", m_networkViews).t();
  S.row(1) = monitorSpkStrength("Tyrion Lannister", m_networkViews).t();
  S.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/S.dat", arma::raw_ascii);
  
  m_networkViews = buildCumNetworks(10);

  /*
    arma::mat IW;
    IW.zeros(3, m_networkViews.size());
    IW.row(0) = (monitorRelationWeight("Francis Underwood", "Claire Underwood", m_networkViews)).t();
    IW.row(1) = (monitorRelationWeight("Francis Underwood", "Martin Spinella", m_networkViews)).t();
    IW.row(2) = (monitorRelationWeight("Lucas Goodwin", "Gavin Orsay", m_networkViews)).t();
    IW.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/IW10.dat", arma::raw_ascii);
  */

  arma::mat IS;

  IS.zeros(2, m_networkViews.size());

  /*
  IS.row(0) = (monitorSpkStrength("Walter White", m_networkViews)).t();
  IS.row(1) = (monitorSpkStrength("Tuco Salamanca", m_networkViews)).t();
  IS.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/IS10.dat", arma::raw_ascii);
  */
  
  IS.row(0) = monitorSpkStrength("Daenerys Targaryen", m_networkViews).t();
  IS.row(1) = monitorSpkStrength("Tyrion Lannister", m_networkViews).t();
  IS.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/IS10.dat", arma::raw_ascii);

  m_networkViews = buildCumNetworks(40);

  /*
    IW.zeros(3, m_networkViews.size());
    IW.row(0) = (monitorRelationWeight("Francis Underwood", "Claire Underwood", m_networkViews)).t();
    IW.row(1) = (monitorRelationWeight("Francis Underwood", "Martin Spinella", m_networkViews)).t();
    IW.row(2) = (monitorRelationWeight("Lucas Goodwin", "Gavin Orsay", m_networkViews)).t();
    IW.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/IW40.dat", arma::raw_ascii);
  */

  IS.zeros(2, m_networkViews.size());

  /*
  IS.row(0) = (monitorSpkStrength("Walter White", m_networkViews)).t();
  IS.row(1) = (monitorSpkStrength("Tuco Salamanca", m_networkViews)).t();
  IS.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/IS40.dat", arma::raw_ascii);
  */

  IS.row(0) = monitorSpkStrength("Daenerys Targaryen", m_networkViews).t();
  IS.row(1) = monitorSpkStrength("Tyrion Lannister", m_networkViews).t();
  IS.save("/home/xbost/Outils/tv_series_proc_tool/tools/sna/matlab/data/IS40.dat", arma::raw_ascii);
}

arma::vec SocialNetProcessor::monitorRelationWeight(const QString &fSpk, const QString &sSpk, const QVector<QMap<QString, QMap<QString, qreal> > > &netViews)
{
  arma::vec W;
  W.zeros(netViews.size());

  for (int i(0); i < netViews.size(); i++) {
   
    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = netViews[i].begin();

    while (it1 != netViews[i].end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interLocs = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interLocs.begin();

      while (it2 != interLocs.end()) {

	QString sSpeaker = it2.key();
	qreal weight = it2.value();

	if ((fSpeaker == fSpk && sSpeaker == sSpk) ||
	    (fSpeaker == sSpk && sSpeaker == fSpk))
	  W(i) += weight;

	it2++;
      }

      it1++;
    }
  }

  return W;
}

arma::vec SocialNetProcessor::monitorSpkStrength(const QString &spk, const QVector<QMap<QString, QMap<QString, qreal> > > &netViews)
{
  arma::vec S;
  S.zeros(netViews.size());

  for (int i(0); i < netViews.size(); i++) {
   
    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = netViews[i].begin();

    while (it1 != netViews[i].end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interLocs = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interLocs.begin();

      while (it2 != interLocs.end()) {

	QString sSpeaker = it2.key();
	qreal weight = it2.value();

	if (fSpeaker == spk || sSpeaker == spk)
	  S(i) += weight;

	it2++;
      }

      it1++;
    }
  }

  return S;
}

void SocialNetProcessor::displaySpeakerNeighbors(const QString &speaker, int step, int nbToDisp)
{
  QList<QPair<qreal, QString> > currNeighbors;
  qreal currStrength(0.0);
  QStringList interactingSpeakers;

  for (int i(0); i < m_networkViews.size(); i += step) {

    currNeighbors = getSpeakerNeighbors(speaker, m_networkViews[i]);
    currStrength = getSpeakerStrength(currNeighbors);
    interactingSpeakers = getInteractingSpeakers(m_sceneInteractions[i]);

    if (interactingSpeakers.contains(speaker)) {
	
      qDebug() << i << interactingSpeakers;
      
      for (int j(0); j < currNeighbors.size(); j++)
	if (j < nbToDisp)
	  qDebug() << i << currStrength << speaker << "->" << currNeighbors[j].second << currNeighbors[j].first;
      qDebug();
    }
  }
}

qreal SocialNetProcessor::getSpeakerStrength(const QList<QPair<qreal, QString> > &neighbors)
{
  qreal strength(0.0);

  for (int i(0); i < neighbors.size(); i++)
    strength += neighbors[i].first;

  return strength;
}

QList<QPair<qreal, QString> > SocialNetProcessor::getSpeakerNeighbors(const QString &speaker, const QMap<QString, QMap<QString, qreal> > &edges)
{
  QList<QPair<qreal, QString> > neighbors;

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();

      if (fSpeaker == speaker)
	neighbors.push_back(QPair<qreal, QString>(w, sSpeaker));
      else if (sSpeaker == speaker)
	neighbors.push_back(QPair<qreal, QString>(w, fSpeaker));

      it2++;
    }
    it1++;
  }

  std::sort(neighbors.begin(), neighbors.end());
  std::reverse(neighbors.begin(), neighbors.end());

  return neighbors;
}

arma::mat SocialNetProcessor::getSocialStatesDist(const QString &speaker, QList<QPair<int, QList<QPair<QString, qreal > > > > &socialStates)
{
  arma::mat D;
  QList<arma::vec> viewNeighbors;

  // scene indices for speaker occurrences
  int n(0);

  for (int i(0); i < m_networkViews.size(); i++) {

    QList<QPair<qreal, QString> > currNeighbors = getSpeakerNeighbors(speaker, m_networkViews[i]);
    QStringList interactingSpeakers = getInteractingSpeakers(m_sceneInteractions[i]);

    if (interactingSpeakers.contains(speaker)) {
	
      QList<QPair<QString, qreal > > socialState;
      
      for (int j(0); j < currNeighbors.size(); j++)
	socialState.push_back(QPair<QString, qreal>(currNeighbors[j].second, currNeighbors[j].first));

      socialStates.push_back(QPair<int, QList<QPair<QString, qreal > > >(i, socialState));

      arma::vec V = vectorizeNeighbors(speaker, m_networkViews[i]);
      // arma::vec V = simToNeighborsNeighborhood(speaker, m_networkViews[i]);
      viewNeighbors.push_back(V);

      n++;
    }
  }

  D.zeros(viewNeighbors.size(), viewNeighbors.size());
  for (int i(0); i < viewNeighbors.size() - 1; i++)
    for (int j(i+1); j < viewNeighbors.size(); j++) {
      D(i, j) = l2Dist(viewNeighbors[i], viewNeighbors[j]);
      D(j, i) = D(i, j);
    }

  return D;
}

QList<QPair<QString, QString> > SocialNetProcessor::getEdgesEnds()
{
  QList<QPair<QString, QString> > edgesEnds;

  if (!m_networkViews.isEmpty()) {

    QMap<QString, QMap<QString, qreal> > edges = m_networkViews.first();

    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

    it1 = edges.begin();

    while (it1 != edges.end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interlocs = it1.value();
    
      QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

      while (it2 != interlocs.end()) {
	
	QString sSpeaker = it2.key();
	edgesEnds.push_back(QPair<QString, QString>(fSpeaker, sSpeaker));

	it2++;
      }
      it1++;
    }
  }

  return edgesEnds;
}

arma::vec SocialNetProcessor::vectorizeNeighbors(const QString &speaker, const QMap<QString, QMap<QString, qreal> > &edges)
{
  arma::vec V;

  // number of components
  int n(0);
  
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {
    n += it1.value().size();
    it1++;
  }

  V.zeros(n);

  // set components
  it1 = edges.begin();

  int i(0);
  while (it1 != edges.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();

      if (fSpeaker == speaker || sSpeaker == speaker)
	V(i) = w;

      i++;
      it2++;
    }
    it1++;
  }

  return V;
}

arma::vec SocialNetProcessor::simToNeighborsNeighborhood(const QString &speaker, const QMap<QString, QMap<QString, qreal> > &edges)
{
  arma::vec V;

  // number of components
  int n(0);
  
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = edges.begin();

  while (it1 != edges.end()) {
    n += it1.value().size();
    it1++;
  }

  V.zeros(n);
  
  arma::vec N = vectorizeNeighbors(speaker, edges);

  // set components
  it1 = edges.begin();

  int i(0);
  while (it1 != edges.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      arma::vec NN;

      if (fSpeaker == speaker)
	NN = vectorizeNeighbors(sSpeaker, edges);
      else if (sSpeaker == speaker)
	NN = vectorizeNeighbors(fSpeaker, edges);

      V(i) = cosineSim(N, NN);

      i++;
      it2++;
    }
    it1++;
  }

  return V;
}

/////////////////////
// narrative chart //
/////////////////////

void SocialNetProcessor::setNarrChart()
{
  // speakers to focus on
  /*
  QStringList selSpeakers = {"Bronn",
			     "Tyrion Lannister",
			     "Barristan Selmy",
			     "Daenerys Targaryen",
			     "Jorah Mormont",
			     "Jaime Lannister",
			     "Robb Stark",
			     "Brienne of Tarth",
			     "Jon Snow",
			     "Samwell Tarly",
			     "Ygritte"
  };
  std::sort(selSpeakers.begin(), selSpeakers.end());
  */

  // scene interval
  int jMin(0);
  int jMax(m_networkViews.size() - 1);

  QStringList selSpeakers = filterSpeakers(jMin, jMax, 0.5);

  // initialize narrative char
  for (int i(0); i < selSpeakers.size(); i++)
    m_narrChart[selSpeakers[i]] = QVector<QPoint>(jMax-jMin+1, QPoint(-1, -1));

  // setNarrChart_aux(jMin, jMax, jMin, S, selSpeakers, m_narrChart);
  setNarrChart_aux_bis(jMin, jMax, selSpeakers);
}

void SocialNetProcessor::setNarrChart_aux_bis(int inf, int sup, const QStringList &speakers)
{
  // number of snapshots
  int T = sup - inf + 1;

  // parameters
  int nIter = 1;
  qreal refSpkWeight = 1.0;
  QVector<int> initPos(speakers.size());

  // initialize narrative chart
  for (int i(0); i < speakers.size(); i++)
    m_narrChart[speakers[i]].resize(T);

  // positions for first snapshot: alphabetical order of speaker label
  // arma::mat A = getMatrix(speakers, inf);
  // m_optimizer->orderStorylines(initPos, A);
  
  for (int i(0); i < initPos.size(); i++) {
    initPos[i] = i;
    m_narrChart[speakers[i]][0] = QPoint(inf, initPos[i]);
  }
  
  for (int k(0); k < 1; k++) {

    // forward populating narrative chart
    for (int j(1); j < T; j++) {

      // qDebug() << "Scene" << k << j;

      QMap<QString, QMap<QString, qreal> > neighbors = snapshotToNeighbors(j + inf);
      QVector<int> pos = m_optimizer->barycenterOrdering(speakers, neighbors, initPos, nIter, refSpkWeight);

      for (int i(0); i < pos.size(); i++)
	m_narrChart[speakers[i]][j] = QPoint(j + inf, pos[i]);

      initPos = pos;
    }

    // backward populating narrative chart
    for (int j(T-2); j >= 0; j--) {

      // qDebug() << "Scene" << k << j;

      QMap<QString, QMap<QString, qreal> > neighbors = snapshotToNeighbors(j + inf);
      QVector<int> pos = m_optimizer->barycenterOrdering(speakers, neighbors, initPos, nIter, refSpkWeight);

      for (int i(0); i < pos.size(); i++)
	m_narrChart[speakers[i]][j] = QPoint(j + inf, pos[i]);

      initPos = pos;
    }

    refSpkWeight += 0.25;
  }
}

arma::mat SocialNetProcessor::getMatrix(const QStringList &speakers, int i)
{
  arma::mat A;
  A.zeros(speakers.size(), speakers.size());

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = m_networkViews[i].begin();

  while (it1 != m_networkViews[i].end()) {

    QString fSpk = it1.key();
    QMap<QString, qreal> interloc = it1.value();

    QMap<QString, qreal>::const_iterator it2 = interloc.begin();

    while (it2 != interloc.end()) {
      
      QString sSpk = it2.key();
      qreal weight  = it2.value();

      if (speakers.contains(fSpk) && speakers.contains(sSpk)) {
	int k = speakers.indexOf(fSpk);
	int l = speakers.indexOf(sSpk);
      
	A(k, l) = weight;
	A(l, k) = A(k, l);
      }

      it2++;
    }

    it1++;
  }

  return A;
}

QMap<QString, QMap<QString, qreal> > SocialNetProcessor::snapshotToNeighbors(int i)
{
  QMap<QString, QMap<QString, qreal> > neighbors;

  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = m_networkViews[i].begin();

  while (it1 != m_networkViews[i].end()) {

    QString fSpk = it1.key();
    QMap<QString, qreal> interloc = it1.value();

    QMap<QString, qreal>::const_iterator it2 = interloc.begin();

    while (it2 != interloc.end()) {
      
      QString sSpk = it2.key();
      qreal weight  = it2.value();

      neighbors[fSpk][sSpk] = weight;
      neighbors[sSpk][fSpk] = weight;

      it2++;
    }

    it1++;
  }

  return neighbors;
}

void SocialNetProcessor::setNarrChart_aux(int inf, int sup, int jMin, const arma::mat &S, const QStringList &selSpeakers, QMap<QString, QVector<QPoint> > &narrChart)
{
  int n = selSpeakers.size();
  int T = sup - inf + 1;
  int med = (inf + sup) / 2;

  if (T < 3)
    return;

  qDebug() << inf << med << sup;

  // edge weights in first, median and last scenes
  arma::mat IA = S.submat(0,
			  inf * selSpeakers.size(), 
			  selSpeakers.size() - 1,
			  (inf + 1) * selSpeakers.size() - 1);
  
  arma::mat FA = S.submat(0,
			  sup * selSpeakers.size(), 
			  selSpeakers.size() - 1,
			  (sup + 1) * selSpeakers.size() - 1);

  arma::mat MA = S.submat(0,
			  med * selSpeakers.size(), 
			  selSpeakers.size() - 1,
			  (med + 1) * selSpeakers.size() - 1);

  // permutations from previous iterations
  QVector<QVector<int> > prevPos(3);
  
  // narrative chart already initialized
  if (narrChart[selSpeakers[0]].first() != QPoint(-1, -1)) {
    prevPos[0] = posFromNarrChart(inf, selSpeakers, narrChart);
    prevPos[2] = posFromNarrChart(sup, selSpeakers, narrChart);
  }

  // best permutation + alignment in median scene
  QVector<QVector<int> > pos;
  arma::mat A = join_horiz(IA, join_horiz(MA, FA));
  m_optimizer->orderStorylines_global(pos, A, prevPos, 0.3);

  qDebug() << pos;
  
  // set narrative chart
  for (int i(0); i < pos.size(); i++)
    for (int t(inf); t <= sup; t++) { 
      if (t < med)
	narrChart[selSpeakers[i]][t-jMin] = QPoint(t, pos[i][0]);
      else if (t == med)
	narrChart[selSpeakers[i]][t-jMin] = QPoint(t, pos[i][1]);
      else
	narrChart[selSpeakers[i]][t-jMin] = QPoint(t, pos[i][2]);
    }
  
  // setNarrChart_aux(inf, med, jMin, S, selSpeakers, narrChart);
  // setNarrChart_aux(med, sup, jMin, S, selSpeakers, narrChart);
}

QVector<int> SocialNetProcessor::posFromNarrChart(int index, const QStringList &selSpeakers, QMap<QString, QVector<QPoint> > &narrChart)
{
  QVector<int> pos(selSpeakers.size());

  QMap<QString, QVector<QPoint> >::const_iterator it = narrChart.begin();

  while (it != narrChart.end()) {

    QString speaker = it.key();
    QVector<QPoint> points = it.value();

    for (int i(0); i < points.size(); i++)
      if (points[i].x() == index)
	pos[selSpeakers.indexOf(speaker)] = points[i].y();

    it++;
  }

  return pos;
}

QMap<QString, QVector<QPoint> > SocialNetProcessor::getNarrChart() const
{
  return m_narrChart;
}

QStringList SocialNetProcessor::filterSpeakers(int iMin, int iMax, qreal minStrength)
{
  QStringList filSpeakers;
  QMap<QString, qreal> spkStrength;

  for (int i(iMin); i <= iMax; i++) {
    
    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = m_networkViews[i].begin();

    while (it1 != m_networkViews[i].end()) {

      QString fSpk = it1.key();
      QMap<QString, qreal> interloc = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interloc.begin();

      while (it2 != interloc.end()) {
      
	QString sSpk = it2.key();
	qreal weight  = it2.value();

	spkStrength[fSpk] += weight;
	spkStrength[sSpk] += weight;

	it2++;
      }

      it1++;
    }
  }

  QMap<QString, qreal>::const_iterator it = spkStrength.begin();

  while (it != spkStrength.end()) {

    QString spk = it.key();
    qreal strength = it.value() / (iMax - iMin + 1);

    if (strength >= minStrength)
      filSpeakers.push_back(spk);

    it++;
  }

  std::sort(filSpeakers.begin(), filSpeakers.end());

  return filSpeakers;
}
