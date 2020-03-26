#ifndef UTTERANCETREE_H
#define UTTERANCETREE_H

#include <QLineF>
#include <QPointF>

#include <armadillo>

#include "UttTreeNode.h"

class UtteranceTree
{
 public:
  enum DistType {
    L2, Mahal
  };

  enum AgrCrit {
    Min, Max, Mean, Ward
  };

  enum PartMeth {
    Silhouette, Bipartition
  };

  UtteranceTree();
  ~UtteranceTree();

  void clearTree();
  void clearTree(UttTreeNode *node);
  void setTree(const arma::mat &S, const arma::mat &W, const arma::mat &Sigma);
  void setDist(DistType dist);
  void setAgr(AgrCrit agr);
  void setPartMeth(PartMeth partMeth);
  void setDiff(const arma::mat &Diff);
  void displayTree(const arma::umat &map);
  void displayTree(UttTreeNode *node, const arma::umat &map);
  void displayTree(QVector<QString> characters);
  void displayTree(UttTreeNode *node, QVector<QString> characters);
  
  double computeWeight(UttTreeNode *node);
  void getClusterInstances(UttTreeNode *node, QList<UttTreeNode *> &instances);
  int getClusterSize();
  int getClusterSize(UttTreeNode *node);
  void getCoordinates(QList<QPair<QLineF, QPair<int, bool>>> &coord);
  void getCoordinates(UttTreeNode *node, QList<QPair<QLineF, QPair<int, bool>>> &coord, int leftMost, QPointF from);
  DistType getDist();
  void setPartition(qreal height);
  QList<QList<int>> cutTree(UttTreeNode *node, qreal height);
  void cutTree(UttTreeNode *node, double ultDist, QList<UttTreeNode *> &subTrees);
  double getBestCutValue();
  QList<QList<int>> getPartition();
  QVector<qreal> getCutValues();

 private:
  double computeDistance(const arma::mat &U, const arma::mat &V, const arma::mat &SigmaInv);
  arma::mat computeDistMat(const arma::mat &S, const arma::mat &SigmaInv);
  arma::mat updateDistances(QList<UttTreeNode *> clusters, UttTreeNode *merged1, UttTreeNode *merged2, arma::uword iMin, arma::uword iMax, const arma::mat &D);
  arma::mat updateDistancesWard(const arma::mat &S, QList<UttTreeNode *> clusters, UttTreeNode *newCluster, arma::uword iMin, arma::uword iMax);
  arma::mat retrieveCentroid(const arma::mat &S);
  arma::mat computeDeltaI(const arma::mat &D, const arma::mat &W);
  arma::mat computeClusterCenter(const arma::mat &S, const arma::umat &Idx);
  void retrieveUltDist(UttTreeNode *node, QList<qreal> &ultDists);
  int getBestPartIdxSil(QList<QList<QList<int>>> partitions);
  void displayClusters(QList<UttTreeNode *> clusters);
  double getBestCutValue(UttTreeNode *node);

  QList<QList<int>> m_partition;
  QVector<qreal> m_cutValues;
  UttTreeNode *m_root;
  DistType m_dist;
  AgrCrit m_agr;
  PartMeth m_partMeth;
  arma::mat m_D;
  arma::mat m_Diff;
  arma::mat m_SigmaInv;
};

#endif
