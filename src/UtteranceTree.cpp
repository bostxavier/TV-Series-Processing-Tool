#include <QDebug>
#include <QList>
#include <cmath>
#include <iostream>

#include "UtteranceTree.h"

using namespace arma;

UtteranceTree::UtteranceTree()
  : m_root(nullptr),
    m_dist(Mahal),
    m_agr(Ward),
    m_partMeth(Silhouette)
{
}

UtteranceTree::~UtteranceTree()
{
  clearTree(m_root);
  m_root = nullptr;
}

void UtteranceTree::clearTree()
{
  clearTree(m_root);
  m_root = nullptr;
}

void UtteranceTree::clearTree(UttTreeNode *node)
{
  if (node != nullptr) {
    clearTree(node->getLeftSon());
    clearTree(node->getRightSon());
    delete node;
  }
}

void UtteranceTree::displayTree(const umat &map)
{
  displayTree(m_root, map);
}

void UtteranceTree::displayTree(UttTreeNode *node, const umat &map)
{
  if (node != nullptr) {
    if (node->getUltDist() > 0)
      qDebug() << node->getUltDist();
    if (node->getSubRef() != -1)
      qDebug() << map(node->getSubRef());
    displayTree(node->getLeftSon(), map);
    displayTree(node->getRightSon(), map);
  }
}

void UtteranceTree::displayTree(QVector<QString> characters)
{
  displayTree(m_root, characters);
}

void UtteranceTree::displayTree(UttTreeNode *node, QVector<QString> characters)
{
  if (node != nullptr) {
    if (node->getUltDist() > 0)
      qDebug() << node->getUltDist();
    if (node->getSubRef() != -1)
      qDebug() << characters[node->getSubRef()];
    displayTree(node->getLeftSon(), characters);
    displayTree(node->getRightSon(), characters);
  }
}

qreal UtteranceTree::computeWeight(UttTreeNode *node)
{
  if (node == nullptr)
    return 0.0;

  return node->getWeight() + computeWeight(node->getLeftSon()) + computeWeight(node->getRightSon());
}

void UtteranceTree::getClusterInstances(UttTreeNode *node, QList<UttTreeNode *> &instances)
{
  if (node != nullptr) {
    if (node->getSubRef() != -1)
      instances.push_back(node);
    getClusterInstances(node->getLeftSon(), instances);
    getClusterInstances(node->getRightSon(), instances);
  }
}

int UtteranceTree::getClusterSize()
{
  return getClusterSize(m_root);
}

int UtteranceTree::getClusterSize(UttTreeNode *node)
{
  if (node == nullptr)
    return 0;

  if (node->getSubRef() != -1)
    return 1;

  return getClusterSize(node->getLeftSon()) + getClusterSize(node->getRightSon());
}

QList<QList<int>> UtteranceTree::getPartition()
{
  return m_partition;
}


QVector<qreal> UtteranceTree::getCutValues()
{
  return m_cutValues;
}


void UtteranceTree::getCoordinates(QList<QPair<QLineF, QPair<int, bool>>> &coord)
{
  getCoordinates(m_root, coord, 0, QPointF());
}

void UtteranceTree::getCoordinates(UttTreeNode *node, QList<QPair<QLineF, QPair<int, bool>>> &coord, int leftMost, QPointF from)
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

    // add line from previous node and associated subtitle reference
    QPointF to(x, y);
    QPair<int, bool> nodeFeatures(node->getSubRef(), node->getVisible());
    coord.push_back(QPair<QLineF, QPair<int, bool>>(QLineF(from, to), nodeFeatures));
    
    getCoordinates(node->getLeftSon(), coord, leftMost, QPointF(x, y));
    getCoordinates(node->getRightSon(), coord, leftMost + lSize, QPointF(x, y));
  }
}

///////////////
// modifiers //
///////////////

void UtteranceTree::setTree(const mat &S, const mat &W, const mat &SigmaInv)
{
  qreal d(0.0);             // minimum distance in distance matrix
  mat N;                    // updated distance from new cluster
  mat M;                    // updated distance from new cluster
  uword toMerge1, toMerge2; // indices of the closest two instances
  uword iMin, iMax;         // min and max indices of the instances to merge

  // freeing memory used by current tree
  clearTree();
  m_cutValues.clear();
  m_partition.clear();
    
  // inverse of covariance matrix
  m_SigmaInv = SigmaInv;

  // computing distance matrix between instances
  mat D = computeDistMat(S, SigmaInv);

  // using DeltaI matrix in case of Ward criterion
  if (m_agr == Ward)
    D = computeDeltaI(D, W);

  // copying distance matrix into member attribute
  m_D = D;

  /*************************************/
  /* hierarchical clustering algorithm */
  /*************************************/

  if (D.n_rows > 0) {

    // stop condition
    umat F = find(D != datum::inf);

    // initializing list of clusters
    QList<UttTreeNode *> clusters;
    for (uword i(0); i < S.n_rows; i++)
      clusters.push_back(new UttTreeNode(0.0, W(i), i));

    while (!F.is_empty()) {

      // retrieving indices of the closest two instances
      d = D.min(toMerge1, toMerge2);

      // ordering the indices of instances to merge for deleting purpose
      if (toMerge1 < toMerge2) {
	iMin = toMerge1;
	iMax = toMerge2;
      }
      else {
	iMin = toMerge2;
	iMax = toMerge1;
      }

      // creation of the new cluster
      UttTreeNode *newCluster = new UttTreeNode(d, 0.0, -1, clusters[iMin], clusters[iMax]);

      // re-estimating distances from cluster of agregated instances
      N = updateDistances(clusters, clusters[iMin], clusters[iMax], iMin, iMax, D);

      // updating list of clusters
      clusters.removeAt(iMax);
      clusters.removeAt(iMin);
      clusters.push_back(newCluster);

      /*
      qDebug();
      cout << D;
      
      qDebug() << "(" << iMin + 1 << "," << iMax + 1 << ")";
      displayClusters(clusters);
      qDebug();
      */

      // updating distance matrix
      D.shed_row(iMax);
      D.shed_col(iMax);
      D.shed_row(iMin);
      D.shed_col(iMin);
      D.insert_cols(D.n_cols, N);
      mat T;
      T << datum::inf;
      N.insert_rows(N.n_rows, T);
      D.insert_rows(D.n_rows, N.t());
      
      /*
      cout << D;
      qDebug();
      */

      // updating flag
      F = find(D != datum::inf);
    }

    // setting optimal partition
    for (int i(0); i < clusters.size(); i++) {
      qreal best = getBestCutValue(clusters[i]);
      m_cutValues.push_back(best);
      m_partition.append(cutTree(clusters[i], best));
    }

    // incomplete tree due to constrained clustering:
    // artificially merge remaining clusters
    d += d / 2;
    while (clusters.size() > 1) {

      clusters[0]->setVisible(false);
      clusters[1]->setVisible(false);

      UttTreeNode *newCluster = new UttTreeNode(d, 0.0, -1, clusters[0], clusters[1], false);
      clusters.removeFirst();
      clusters.removeFirst();
      clusters.push_front(newCluster);
    }

    if (clusters.size() > 0) {
      m_root = clusters[0];
    }
  }
}

void UtteranceTree::displayClusters(QList<UttTreeNode *> clusters)
{
  for (int i(0); i < clusters.size(); i++) {
    QList<UttTreeNode *> instances;
    getClusterInstances(clusters[i], instances);
    for (int j(0); j < instances.size(); j++) {
      std::cout << (instances[j]->getSubRef() + 1) << " ";
    }
    std::cout << endl;
  }
}

void UtteranceTree::setPartition(qreal ultDist)
{
  m_cutValues.clear();
  m_cutValues.push_back(ultDist);
  m_partition = cutTree(m_root, ultDist);
}

QList<QList<int>> UtteranceTree::cutTree(UttTreeNode *node, qreal ultDist)
{
  QList<QList<int>> partition;
  QList<UttTreeNode *> subTrees;
  
  cutTree(node, ultDist, subTrees);

  for (int i(0); i < subTrees.size(); i++) {

    QList<UttTreeNode *> leaves;
    QList<int> part;
    getClusterInstances(subTrees[i], leaves);

    for (int j(0); j < leaves.size(); j++) 
      part.push_back(leaves[j]->getSubRef());

    partition.push_back(part);
  }

  return partition;
}

void UtteranceTree::cutTree(UttTreeNode *node, qreal ultDist, QList<UttTreeNode *> &subTrees)
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

void UtteranceTree::setDist(DistType dist)
{
  m_dist = dist;
}

void UtteranceTree::setAgr(AgrCrit agr)
{
  m_agr = agr;
}

void UtteranceTree::setPartMeth(PartMeth partMeth)
{
  m_partMeth = partMeth;
}

void UtteranceTree::setDiff(const mat &Diff)
{
  m_Diff = Diff;
}


UtteranceTree::DistType UtteranceTree::getDist()
{
  return m_dist;
}

qreal UtteranceTree::getBestCutValue()
{
  return getBestCutValue(m_root);
}

qreal UtteranceTree::getBestCutValue(UttTreeNode *node)
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

    // call appropriate method to evaluate partitions
    switch (m_partMeth) {
    case Silhouette:
      bestIdx = getBestPartIdxSil(partitions);
      break;
    case Bipartition:
      bestIdx = 1;
      break;
    }
  
  if (bestIdx == -1)
    return 0.0;

  return cutDists[bestIdx];
}

///////////////////////
// auxiliary methods //
///////////////////////

qreal UtteranceTree::computeDistance(const mat &U, const mat &V, const mat &SigmaInv)
{
  qreal d(0.0);
  mat diff;

  switch (m_dist) {
  case L2:
    d = norm(U - V);
    break;
  case Mahal:
    diff = U - V;
    d = as_scalar(sqrt(diff * SigmaInv * diff.t()));
    break;
  }

  return d;
}

mat UtteranceTree::computeDistMat(const mat &S, const mat &SigmaInv)
{
  mat D(S.n_rows, S.n_rows, fill::zeros);
  qreal d;

  for (uword i(0); i < S.n_rows; i++) {
    D(i, i) = datum::inf;
    for (uword j(i+1); j < S.n_rows; j++) {
      d = computeDistance(S.row(i), S.row(j), SigmaInv);
      D(i, j) = d;
      D(j, i) = d;
    }
  }
  
  if (D.n_rows == m_Diff.n_rows && D.n_cols == m_Diff.n_cols)
    return (D % m_Diff);

  return D;
}

mat UtteranceTree::updateDistancesWard(const mat &S, QList<UttTreeNode *> clusters, UttTreeNode *newCluster, uword iMin, uword iMax)
{
  // updated distances from new cluster
  mat N(clusters.size(), 1, fill::zeros);

  // weights of merged clusters and new cluster
  qreal newWeight(computeWeight(newCluster));

  // instances in the new cluter
  QList<UttTreeNode *> newInst;
  getClusterInstances(newCluster, newInst);

  // indices of new cluster instances in S matrix
  umat Idx(1, newInst.size());
  for (int i(0); i < newInst.size(); i++)
    Idx(0, i) = newInst[i]->getSubRef();

  // mass center of the new cluster
  mat g1 = computeClusterCenter(S, Idx);

  for (uword i(0); i < static_cast<uword>(clusters.size()); i++)

    if (i != iMin && i != iMax) {

      qreal currWeight(computeWeight(clusters[i]));

      QList<UttTreeNode *> currInst;
      getClusterInstances(clusters[i], newInst);

      // indices of current cluster instances in S matrix
      umat currIdx(1, newInst.size());
      for (int j(0); j < newInst.size(); j++)
	currIdx(0, j) = newInst[j]->getSubRef();

      // mass center of the new cluster
      mat g2 = computeClusterCenter(S, currIdx);

      N(i, 0) = (newWeight * currWeight) / (newWeight + currWeight) * pow(computeDistance(g1, g2, m_SigmaInv), 2);

      if (!is_finite(N.row(i)))
	N(i, 0) = datum::inf;
    }

  // removing rows corresponding to clustered instances
  N.shed_row(iMax);
  N.shed_row(iMin);

  return N;
}

mat UtteranceTree::computeClusterCenter(const mat &S, const umat &Idx)
{
  // mass center
  mat g(1, S.n_cols);

  // matrix of cluster instances
  mat I = S.rows(Idx);
  g = sum(I) / I.n_rows;
  
  return g;
}

mat UtteranceTree::updateDistances(QList<UttTreeNode *> clusters, UttTreeNode *merged1, UttTreeNode *merged2, uword iMin, uword iMax, const mat &D)
{
  // updated distances from new cluster
  mat N(clusters.size(), 1, fill::zeros);

  // weights of merged clusters and new cluster
  qreal weight1(computeWeight(merged1));
  qreal weight2(computeWeight(merged2));
  qreal totWeight = weight1 + weight2;

  // weight of current cluster
  qreal currWeight;

  // coefficients of Lance-Wiliams formula
  qreal alpha1(0.0), alpha2(0.0), beta(0.0), gamma(0.0);

  // initializing coefficients in case of min/max/mean criteria
  switch (m_agr) {
  case Min:
    alpha2 = alpha1 = 1.0 / 2.0;
    gamma = - 1.0 / 2.0;
    break;
  case Max:
    alpha2 = alpha1 = 1.0 / 2.0;
    gamma = 1.0 / 2.0;
    break;
  case Mean:
    alpha1 = weight1 / totWeight;
    alpha2 = weight2 / totWeight;
    break;
  case Ward:
    break;
  }
  
  // looping over clusters
  for (uword i(0); i < static_cast<uword>(clusters.size()); i++)

    if (i != iMin && i != iMax) {

      // initializing coefficients in case of ward criterion
      if (m_agr == Ward) {
	currWeight = computeWeight(clusters[i]);
	alpha1 = (currWeight + weight1) / (currWeight + totWeight);
	alpha2 = (currWeight + weight2) / (currWeight + totWeight);
	beta = - currWeight / (currWeight + totWeight);
      }
     
      N(i, 0) = alpha1 * D(iMin, i) + alpha2 * D(iMax, i) + beta * D(iMin, iMax) + gamma * std::abs(D(iMin, i) - D(iMax, i));

      if (!is_finite(N.row(i)))
	N(i, 0) = datum::inf;
    }

  // removing rows corresponding to clustered instances
  N.shed_row(iMax);
  N.shed_row(iMin);

  return N;
}

mat UtteranceTree::retrieveCentroid(const arma::mat &S)
{
  return sum(S, 0) / S.n_rows;
}

mat UtteranceTree::computeDeltaI(const mat &D, const mat &W)
{
  qreal wardDist;
  mat DeltaI(pow(D, 2));

  for (uword i(0); i < DeltaI.n_rows; i++) {
    DeltaI(i, i) = datum::inf;
    for (uword j(i+1); j < DeltaI.n_cols; j++) {
      wardDist = W(i) * W(j) * DeltaI(i, j);
      DeltaI(i, j) = wardDist;
      DeltaI(j, i) = wardDist;
    }
  }

  if (DeltaI.n_rows == m_Diff.n_rows && DeltaI.n_cols == m_Diff.n_cols)
    return (DeltaI % m_Diff);

  return DeltaI;
}

void UtteranceTree::retrieveUltDist(UttTreeNode *node, QList<qreal> &ultDists)
{
  qreal currDist;

  if (node != nullptr && !ultDists.contains((currDist = node->getUltDist()))) {
    ultDists.push_back(currDist);
    retrieveUltDist(node->getLeftSon(), ultDists);
    retrieveUltDist(node->getRightSon(), ultDists);
  }
}

int UtteranceTree::getBestPartIdxSil(QList<QList<QList<int>>> partitions)
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
