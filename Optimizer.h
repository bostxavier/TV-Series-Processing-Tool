#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <QWidget>

#include <ilcplex/ilocplex.h>
#include <armadillo>

#include "Dendogram.h"

#include <QDebug>

class Optimizer: public QWidget
{
  Q_OBJECT

 public:
  enum AgrCrit {
    Min, Max, Mean, Ward
  };

  Optimizer(QWidget *parent = 0);

  Dendogram * hierarchicalClustering(arma::mat D, bool temporalCst = false);
  arma::vec updateDistances(QList<DendogramNode *> clusters, DendogramNode *merged1, DendogramNode *merged2, arma::uword iMin, arma::uword iMax, const arma::mat &D, AgrCrit agr);
  qreal computeWeight(DendogramNode *node);
  void displayClusters(QList<DendogramNode *> clusters);
  QList<int> computeClusterMedians(QList<DendogramNode *> clusters);
  void getClusterInstances(DendogramNode *node, QList<DendogramNode *> &instances);

  qreal optimalMatching_bis(const arma::mat &A, QVector<int> &matching);
  QMap<QString, QString> optimalMatching(QList<QString> &nodeLabels_1, QList<QString> &nodelabels_2, QMap<QString, QMap<QString, qreal> > &edgeWeights);

 int setCovering(arma::umat &A, QList<QList<int> > &partition, QList<int> &cIdx);

 qreal pCenter(int p, arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx);
 qreal pCenter_bis(int p, arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx, QList<QPair<int, int> > &ambiguous);
 qreal pCenterOptP(arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx);

 qreal pMedian(int p, arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx);

 qreal coClusterOptMatch(int p, arma::mat D, QList<QList<int> > &shotPartition, QList<int> &shotCIdx, arma::mat A, int pp, arma::mat DP, QList<QList<int> > &utterPartition, QList<int> &utterCIdx, QMap<int, QList<int> > &mapping, qreal lambda);
 qreal coClusterOptMatch_iter(int p, arma::mat D, QList<QList<int> > &shotPartition, QList<int> &shotCIdx, arma::mat A, int pp, arma::mat DP, QList<QList<int> > &utterPartition, QList<int> &utterCIdx, QMap<int, QList<int> > &mapping, qreal alpha);

 qreal orderStorylines(QVector<int> &pos, const arma::mat &A, const QVector<int> &prevPos = QVector<int>());
 qreal orderStorylines_global(QVector<QVector<int> > &pos, const arma::mat &A, const QVector<QVector<int> > &prevPos, qreal alpha = 1.0); 
 QVector<int> barycenterOrdering(const QStringList &speakers, const QMap<QString, QMap<QString, qreal> > &neighbors, const QVector<int> &initPos = QVector<int>(), int nIter = 6, qreal refSpkWeight = 0.25);

 qreal knapsack(const arma::vec &PV, const arma::vec &WV, const arma::mat &FR, const arma::mat &SR, qreal W, QList<int> &selected);
 qreal facilityLocation(const arma::vec &PV, const arma::vec &WV, const arma::mat &FR, const arma::mat &SR, qreal W, QList<int> &selected);
 qreal mmr(arma::vec PV, const arma::vec &WV, const arma::mat &FR, const arma::mat &SR, qreal W, QList<int> &selected);
 void randomSelection(const arma::vec &WV, const arma::mat &FR, qreal W, QList<int> &selected);
 bool isFeasible(int idx, const QList<int> &selected, const arma::vec &WV, const arma::mat &FR, qreal W);
 QVector<QPair<qreal, int> > getRatios(const arma::vec &R);

 public slots:
    
    signals:
      
    private:
};

#endif
