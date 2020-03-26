#include <QDebug>
#include <QPointF>
#include <QLineF>

#include "Dendogram.h"
#include "Optimizer.h"

using namespace std;
using namespace arma;

Dendogram::Dendogram()
  : m_root(nullptr),
    m_bestCutValue(0)
{
}

Dendogram::~Dendogram()
{
  clearTree(m_root);
  m_root = nullptr;
}

void Dendogram::clearTree(DendogramNode *node)
{
  if (node != nullptr) {
    clearTree(node->getLeftSon());
    clearTree(node->getRightSon());
    delete node;
  }
}

void Dendogram::displayTree()
{
  displayTree(m_root);
}

void Dendogram::displayTree(DendogramNode *node)
{
  if (node != nullptr) {
    if (node->getUltDist() > 0)
      qDebug() << node->getUltDist();
    if (node->getLabel() != -1)
      qDebug() << node->getLabel();
    displayTree(node->getLeftSon());
    displayTree(node->getRightSon());
  }
}

int Dendogram::getClusterSize()
{
  return getClusterSize(m_root);
}

int Dendogram::getClusterSize(DendogramNode *node)
{
  if (node == nullptr)
    return 0;

  if (node->getLabel() != -1)
    return 1;

  return getClusterSize(node->getLeftSon()) + getClusterSize(node->getRightSon());
}

void Dendogram::getCoordinates(QList<QPair<QLineF, QPair<int, bool> > > &coord)
{
  getCoordinates(m_root, coord, 0, QPointF());
}

void Dendogram::getCoordinates(DendogramNode *node, QList<QPair<QLineF, QPair<int, bool> > > &coord, int leftMost, QPointF from)
{
  qreal x;
  qreal y;
  int lSize;

  if (node != nullptr) {

    // number of nodes in left child cluster
    lSize = getClusterSize(node->getLeftSon());

    // current y coordinate
    y = node->getUltDist();

    // current x coordinate
    if (y == 0.0)
      x = leftMost;
    else
      x = leftMost + lSize - 0.5;

    // add line from previous node and associated label
    QPointF to(x, y);
    QPair<int, bool> nodeFeatures(node->getLabel(), node->getVisible());
    coord.push_back(QPair<QLineF, QPair<int, bool> >(QLineF(from, to), nodeFeatures));
    
    getCoordinates(node->getLeftSon(), coord, leftMost, QPointF(x, y));
    getCoordinates(node->getRightSon(), coord, leftMost + lSize, QPointF(x, y));
  }
}

void Dendogram::getClusterInstances(DendogramNode *node, QList<DendogramNode *> &instances)
{
  if (node != nullptr) {
    if (node->getLabel() != -1)
      instances.push_back(node);
    getClusterInstances(node->getLeftSon(), instances);
    getClusterInstances(node->getRightSon(), instances);
  }
}

qreal Dendogram::getBestCut() const
{
  return m_bestCutValue;
}

///////////////
// modifiers //
///////////////

void Dendogram::setRoot(DendogramNode *root)
{
  m_root = root;
}

void Dendogram::setDistMat(const arma::mat &D)
{
  m_D = D;
}

////////////////////////
// best set partition //
////////////////////////

void Dendogram::setBestCutValue()
{
  m_bestCutValue = getBestCutValue(m_root);
}

qreal Dendogram::getBestCutValue(DendogramNode *node)
{
  QList<qreal> ultDists;
  QList<qreal> cutDists;
  QList<QList<QList<int>>> partitions;
  int bestIdx(-1);

  // retrieving nodes ultrametric distances
  retrieveUltDist(node, ultDists);

  // deducing possible cut values
  qSort(ultDists.begin(), ultDists.end(), qGreater<qreal>());
  for (int i(0); i < ultDists.size() - 1; i++)
    cutDists.push_back((ultDists[i] + ultDists[i+1]) / 2);

  if (ultDists.size() > 0)
    cutDists.push_front(ultDists[0] + 1);

  // initializing list of possible partitions of the data set
  for (int i(0); i < cutDists.size(); i++) {
    QList<QList<int>> parti = cutTree(node, cutDists[i]);
    partitions.push_back(parti);
  }

  if (partitions.size() > 0)
    bestIdx = getBestPartIdxSil(partitions);
  
  if (bestIdx == -1)
    return 0.0;

  return cutDists[bestIdx];
}

QList<QList<int>> Dendogram::cutTree(DendogramNode *node, qreal ultDist)
{
  QList<QList<int>> partition;
  QList<DendogramNode *> subTrees;
  
  cutTree(node, ultDist, subTrees);

  for (int i(0); i < subTrees.size(); i++) {

    QList<DendogramNode *> leaves;
    QList<int> part;
    getClusterInstances(subTrees[i], leaves);

    for (int j(0); j < leaves.size(); j++) 
      part.push_back(leaves[j]->getLabel());

    partition.push_back(part);
  }

  return partition;
}

void Dendogram::cutTree(DendogramNode *node, qreal ultDist, QList<DendogramNode *> &subTrees)
{
  if (node == nullptr)
    return;

  if (node->getUltDist() <= ultDist) {
    subTrees.push_back(node);
    return;
  }

  cutTree(node->getLeftSon(), ultDist, subTrees);
  cutTree(node->getRightSon(), ultDist, subTrees);
}

void Dendogram::retrieveUltDist(DendogramNode *node, QList<qreal> &ultDists)
{
  qreal currDist;

  if (node != nullptr && !ultDists.contains((currDist = node->getUltDist()))) {
    ultDists.push_back(currDist);
    retrieveUltDist(node->getLeftSon(), ultDists);
    retrieveUltDist(node->getRightSon(), ultDists);
  }
}

int Dendogram::getBestPartIdxSil(QList<QList<QList<int>>> partitions)
{
  int n(partitions[0][0].size()); // number of instances
  QVector<qreal> Q(n);            // quality measure for each partition
  QVector<int> map(n);
  QMap<int, int> mapInv;
  QVector<int> C(n);              // cluster of each instance
  QVector<QVector<qreal>> D(n);   // distance between each instance and each cluster
  QVector<qreal> a(n);            // distance between an instance and its cluster
  QVector<qreal> b(n);            // distance between an instance and the second closest cluster
  QVector<qreal> s(n);            // measures if instance is correctly classified
  int iBest(0);
  Q[iBest] = -1.0;
  
  // saving indices
  for (int i(0); i < n; i++) {
    map[i]  = partitions[0][0][i];
    mapInv[partitions[0][0][i]] = i;
  }

  // looping over the partitions containing (2, ..., n) elements
  for (int i(1); i < partitions.size(); i++) {
    
    // current partition
    QList<QList<int>> partition = partitions[i];

    // for each instance, retrieving its cluster
    for (int j(0); j < n; j++) {
      int k(0);
      bool found(false);
      while (!found && k < partition.size()) {
	if (partition[k].contains(map[j])) {
	  C[j] = k;
	  found = true;
	}
	k++;
      }
    }
    // for each instance, computing its average distance from members of each cluster
    QVector<qreal> d(partition.size());

    // looping over instances
    for (int j(0); j < n; j++) {

      // looping over clusters
      for (int k(0); k < partition.size(); k++) {

	// initializing distance between instance j and cluster k
	d[k] = 0.0;
	int clusterCard = partition[k].size();
 
	// looping over cluster instances
	for (int l(0); l < clusterCard; l++)
	  if (map[j] != partition[k][l]) {
	   
	    d[k] += m_D(map[j], partition[k][l]);
	  }

	// normalizing distance between instance j and cluster k
	int normFac = (k == C[j]) ? clusterCard - 1 : clusterCard;
	d[k] /= normFac;
      }

      // distance between current instance and its cluster
      a[j] = d[C[j]];

      // computing distance between current instance and second closest cluster
      // initialization
      int k(0);
      while (k < partition.size() && k == C[j])
	k++;
      b[j] = d[k];

      // retrieving closest cluster
      for (int k(0); k < partition.size(); k++) {
	if (k != C[j] && d[k] < b[j])
	  b[j] = d[k];
      }

      // computing s(x_i)
      if (partition[C[j]].size() == 1)
	s[j] = 0;
      else
	s[j] = (b[j] - a[j]) / std::max(a[j], b[j]);
    }

    // looping over clusters
    for (int k(0); k < partition.size(); k++) {
      qreal mean = 0.0;

      for (int j(0); j < partition[k].size(); j++) {
	int idx = mapInv[partition[k][j]];
	mean += s[idx];
      }
	
      mean /= partition[k].size();
      Q[i] += partition[k].size() * mean;
    }

    Q[i] /= n;

    if (Q[i] >= Q[iBest])
      iBest = i;
  }

  return iBest;
}

int Dendogram::retrieveSubsetMedoid(const QList<int> &selectedLabels)
{
  int i(-1);

  arma::uvec I;
  I.zeros(selectedLabels.size());

  for (int i(0); i < selectedLabels.size(); i++)
    I(i) = selectedLabels[i];

  // distance submatrix
  arma::mat S = m_D.submat(I, I);

  // retrieve medoid
  Optimizer optimizer;
  QList<QList<int> > partition;
  QList<int> cIdx;
  qreal objValue(0.0);

  objValue = optimizer.pMedian(1, S, partition, cIdx);

  /*
  cout << S << endl;
  qDebug() << partition;
  qDebug() << cIdx;
  qDebug() << objValue;
  */

  return selectedLabels[cIdx.first()];
}
