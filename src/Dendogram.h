#ifndef DENDOGRAM_H
#define DENDOGRAM_H

#include <armadillo>

#include "DendogramNode.h"

class Dendogram
{
  public:
  Dendogram();
  ~Dendogram();

  void setRoot(DendogramNode *root);
  void setDistMat(const arma::mat &D);
  void clearTree(DendogramNode *node);
  void displayTree();
  void displayTree(DendogramNode *node);
  qreal getBestCut() const;
  int getClusterSize();
  int getClusterSize(DendogramNode *node);
  void getClusterInstances(DendogramNode *node, QList<DendogramNode *> &instances);

  void setBestCutValue();
  qreal getBestCutValue(DendogramNode *node);
  QList<QList<int>> cutTree(DendogramNode *node, qreal ultDist);
  void cutTree(DendogramNode *node, qreal ultDist, QList<DendogramNode *> &subTrees);
  void retrieveUltDist(DendogramNode *node, QList<qreal> &ultDists);
  int getBestPartIdxSil(QList<QList<QList<int>>> partitions);

  void getCoordinates(QList<QPair<QLineF, QPair<int, bool> > > &coord);
  void getCoordinates(DendogramNode *node, QList<QPair<QLineF, QPair<int, bool> > > &coord, int leftMost, QPointF from);

  int retrieveSubsetMedoid(const QList<int> &selectedLabels);
  
 private:
  DendogramNode *m_root;
  arma::mat m_D;
qreal m_bestCutValue;
};

#endif
