#include <time.h>
#include <stdlib.h>

#include "Optimizer.h"

using namespace std;
using namespace arma;

#include "DendogramNode.h"

Optimizer::Optimizer(QWidget *parent)
  : QWidget(parent)
{
}

Dendogram * Optimizer::hierarchicalClustering(arma::mat D, bool temporalCst)
{
  Dendogram *dendo = new Dendogram;
  dendo->setDistMat(D);

  qreal d(0.0);         // minimum distance in distance matrix
  mat N;                // updated distance from new cluster
  mat M;                // updated distance from new cluster
  uword toMerge1(0);
  uword toMerge2(0);    // indices of the closest two instances
  uword iMin, iMax;     // min and max indices of the instances to merge

  // set diagonal to +Inf
  for (uword i(0); i < D.n_rows; i++)
    D(i, i) = datum::inf;

  // stop condition
  umat F = arma::find(D != datum::inf);

  // initializing list of clusters
  QList<DendogramNode *> clusters;
  for (uword i(0); i < D.n_rows; i++)
    clusters.push_back(new DendogramNode(0.0, 1.0, i));
  
  while (!F.is_empty()) {

    // retrieving indices of the closest two instances
    if (temporalCst) {
      d = datum::inf;
      for (uword i(0); i < D.n_rows - 1; i++)
	if (D(i, i + 1) < d) {
	  toMerge1 = i;
	  toMerge2 = i + 1;
	  d = arma::as_scalar(D(i, i + 1));
	}
    }
    else
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
    DendogramNode *newCluster = new DendogramNode(d, 0.0, -1, clusters[iMin], clusters[iMax]);
  
    // re-estimating distances from cluster of agregated instances
    N = updateDistances(clusters, clusters[iMin], clusters[iMax], iMin, iMax, D, Ward);
  
    // updating list of clusters
    clusters.removeAt(iMax);
    clusters.removeAt(iMin);

    // compute median values for current clusters
    QList<int> clusterMedians = computeClusterMedians(clusters);

    // compute median value for new cluster
    QList<DendogramNode *> newClusterAsList = {newCluster};
    QList<int> newClusterMedian = computeClusterMedians(newClusterAsList);

    // index of insertion of new cluster
    int iInsert(0);
    while (iInsert < clusterMedians.size() && newClusterMedian.first() > clusterMedians[iInsert])
      iInsert++;

    // qDebug() << clusterMedians << "*" << newClusterMedian << "*" << iInsert;

    clusters.insert(iInsert, newCluster);

    // qDebug();
    // cout << D;
      
    // qDebug() << "(" << iMin << "," << iMax << ")";
    // displayClusters(clusters);
    // qDebug();

    // updating distance matrix
    D.shed_row(iMax);
    D.shed_col(iMax);
    D.shed_row(iMin);
    D.shed_col(iMin);
    D.insert_cols(iInsert, N);
    mat T;
    T << datum::inf;
    N.insert_rows(iInsert, T);
    D.insert_rows(iInsert, N.t());

    // updating flag
    F = arma::find(D != datum::inf);
  }
  
  dendo->setRoot(clusters.first());
  dendo->setBestCutValue();

  return dendo;
}

arma::vec Optimizer::updateDistances(QList<DendogramNode *> clusters, DendogramNode *merged1, DendogramNode *merged2, uword iMin, uword iMax, const mat &D, AgrCrit agr)
{
  // updated distances from new cluster
  mat N;
  N.zeros(clusters.size());

  // weights of merged clusters and new cluster
  qreal weight1(computeWeight(merged1));
  qreal weight2(computeWeight(merged2));
  qreal totWeight = weight1 + weight2;

  // weight of current cluster
  qreal currWeight;

  // coefficients of Lance-Wiliams formula
  qreal alpha1(0.0), alpha2(0.0), beta(0.0), gamma(0.0);

  // initializing coefficients in case of min/max/mean criteria
  switch (agr) {
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
      if (agr == Ward) {
	currWeight = computeWeight(clusters[i]);
	alpha1 = (currWeight + weight1) / (currWeight + totWeight);
	alpha2 = (currWeight + weight2) / (currWeight + totWeight);
	beta = - currWeight / (currWeight + totWeight);
      }
     
      N(i) = alpha1 * D(iMin, i) + alpha2 * D(iMax, i) + beta * D(iMin, iMax) + gamma * qAbs(D(iMin, i) - D(iMax, i));

      if (!is_finite(N.row(i)))
	N(i) = datum::inf;
    }

  // removing rows corresponding to clustered instances
  N.shed_row(iMax);
  N.shed_row(iMin);

  return N;
}

qreal Optimizer::computeWeight(DendogramNode *node)
{
  if (node == nullptr)
    return 0.0;

  return node->getWeight() + computeWeight(node->getLeftSon()) + computeWeight(node->getRightSon());
}

void Optimizer::displayClusters(QList<DendogramNode *> clusters)
{
  for (int i(0); i < clusters.size(); i++) {
    QList<DendogramNode *> instances;
    getClusterInstances(clusters[i], instances);
    for (int j(0); j < instances.size(); j++) {
      std::cout << (instances[j]->getLabel()) << " ";
    }
    std::cout << endl;
  }
}

QList<int> Optimizer::computeClusterMedians(QList<DendogramNode *> clusters)
{
  QList<int> means;

  for (int i(0); i < clusters.size(); i++) {
    
    QList<DendogramNode *> instances;
    getClusterInstances(clusters[i], instances);
    
    arma::uvec U(instances.size());
    
    for (int j(0); j < instances.size(); j++)
      U(j) = instances[j]->getLabel();

    means.push_back(median(U));
  }

  return means;
}

void Optimizer::getClusterInstances(DendogramNode *node, QList<DendogramNode *> &instances)
{
  if (node != nullptr) {
    if (node->getLabel() != -1)
      instances.push_back(node);
    getClusterInstances(node->getLeftSon(), instances);
    getClusterInstances(node->getRightSon(), instances);
  }
}

qreal Optimizer::optimalMatching_bis(const arma::mat &A, QVector<int> &matching)
{
  int p(A.n_rows);
  int n(A.n_cols);
  qreal objValue(0.0);

  matching.fill(-1, p);

  IloEnv env;

  try {
    IloModel model(env);
      
    // var y[i][j] = 1 if node i is assigned to node j; 0 otherwise
    IloArray<IloNumVarArray> y(env);
    for (int i(0); i < p; i++) {
      IloNumVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloNumVar(env, 0.0, 1.0));
      y.add(tmp);
    }

    // coefficient c[i][j]: weight of edge linking node i and node j
    IloArray<IloNumArray> c(env);
    for (int i(0); i < p; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++) {
	tmp.add(A(i, j));
      }
      c.add(tmp);
    }

    // objective function
    IloExpr obj(env);
    for (int i(0); i < p; i++) {
      IloExpr tmp(IloScalProd(c[i], y[i]));
      obj += tmp;
    }
      
    model.add(IloMaximize(env, obj));

    // p constraints (1): each node of the first part must be assigned
    // to at most one node of the second part
    for (int i(0); i < p; i++) {
      IloExpr e1(IloSum(y[i]));
      IloRange c1(e1 <= 1);
      model.add(c1);
    }

    // p constraints (1): each node of the second part must be assigned
    // to at most one node of the first part
    for (int j(0); j < n; j++) {
      IloExpr e2(env);
      for (int i(0); i < p; i++)
	e2 += y[i][j];
      IloRange c2(e2 <= 1);
      model.add(c2);
    }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    // env.out() << "Solution status = " << cplex.getStatus() << endl;
    // env.out() << "Solution value = " << cplex.getObjValue() << endl;

    objValue = cplex.getObjValue();

    // retrieve optimal matching
    for (int i(0); i < p; i++) {
      IloNumArray yVals(env);
      cplex.getValues(yVals, y[i]);
      for (int j(0); j < n; j++)
	if (yVals[j] == 1.0)
	  matching[i] = j;
    }
  }

  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return objValue;
}

QMap<QString, QString> Optimizer::optimalMatching(QList<QString> &nodeLabels_1, QList<QString> &nodeLabels_2, QMap<QString, QMap<QString, qreal> > &edgeWeights)
{
  QMap<QString, QString> matching;
  int p(nodeLabels_1.size());
  int n(nodeLabels_2.size());

  IloEnv env;

  try {
    IloModel model(env);
      
    // var y[i][j] = 1 if node i is assigned to node j; 0 otherwise
    IloArray<IloNumVarArray> y(env);
    for (int i(0); i < p; i++) {
      IloNumVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloNumVar(env, 0.0, 1.0));
      y.add(tmp);
    }

    // coefficient c[i][j]: weight of edge linking node i and node j
    IloArray<IloNumArray> c(env);
    for (int i(0); i < p; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++) {
	tmp.add(edgeWeights[nodeLabels_1[i]][nodeLabels_2[j]]);
      }
      c.add(tmp);
    }

    // objective function
    IloExpr obj(env);
    for (int i(0); i < p; i++) {
      IloExpr tmp(IloScalProd(c[i], y[i]));
      obj += tmp;
    }
      
    model.add(IloMaximize(env, obj));

    // p constraints (1): each node of the first part must be assigned
    // to at most one node of the second part
    for (int i(0); i < p; i++) {
      IloExpr e1(IloSum(y[i]));
      IloRange c1(e1 <= 1);
      model.add(c1);
    }

    // p constraints (1): each node of the second part must be assigned
    // to at most one node of the first part
    for (int j(0); j < n; j++) {
      IloExpr e2(env);
      for (int i(0); i < p; i++)
	e2 += y[i][j];
      IloRange c2(e2 <= 1);
      model.add(c2);
    }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    // env.out() << "Solution status = " << cplex.getStatus() << endl;
    // env.out() << "Solution value = " << cplex.getObjValue() << endl;

    // retrieve optimal matching
    for (int i(0); i < p; i++) {
      IloNumArray yVals(env);
      cplex.getValues(yVals, y[i]);
      for (int j(0); j < n; j++)
	if (yVals[j] == 1.0)
	  matching[nodeLabels_1[i]] = nodeLabels_2[j];
    }
  }

  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return matching;
}

int Optimizer::setCovering(arma::umat &A, QList<QList<int> > &partition, QList<int> &cIdx)
{
  int n(A.n_rows);                  // number of variables
  int nCenters(0);

  IloEnv env;

  try {
    IloModel model(env);
      
    // var x[i] = 1 if instance i is selected as a supply node; 0 otherwise
    IloBoolVarArray x(env);
    for (int i(0); i < n; i++)
      x.add(IloBoolVar(env));

    // distance between nodes
    IloArray<IloBoolArray> a(env);
    for (int i(0); i < n; i++) {
      IloBoolArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(arma::as_scalar(A(i, j)));
      a.add(tmp);
    }

    // objective function
    IloExpr obj(env);

    for (int i(0); i < n; i++) {
      IloExpr e(IloScalProd(a[i], x));
      obj += e;
    }
    
    // obj = IloSum(x);
    
    model.add(IloMinimize(env, obj));

    // n constraints: each demand node must be assigned to at least one supply node
    for (int i(0); i < n; i++) {
      IloExpr e(IloScalProd(a[i], x));
      IloRange c(e >= 1);
      model.add(c);
    }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    // env.out() << "Solution status = " << cplex.getStatus() << endl;
    // env.out() << "Solution value = " << cplex.getObjValue() << endl;
    nCenters = cplex.getObjValue();

    // retrieve center indices
    IloNumArray xVals(env);
    cplex.getValues(xVals, x);

    // retrieve shot partition
    for (int i(0); i < n; i++)
      if (xVals[i] == 1) {
	cIdx.push_back(i);
	arma::umat idx = arma::find(A.col(i));
	QList<int> part;
	for (arma::uword j(0); j < idx.size(); j++)
	  part.push_back(idx(j, 0));

	partition.push_back(part);
      }
  }

  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return nCenters;
}

qreal Optimizer::pCenter(int p, arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx)
{
  int n(D.n_rows);                  // number of nodes
  qreal covDist(-1.0);

  IloEnv env;

  try {

    IloModel model(env);
      
    // var w: maximum distance between a demand node and the nearest facility
    IloNumVar w(env, 0.0);

    // var x[j] = 1 if facility is located at candidate site j; 0 otherwise
    IloBoolVarArray x(env);
    for (int j(0); j < n; j++)
      x.add(IloBoolVar(env));

    // var y[i][j] = 1 if demand node i is allocated to facility located at candidate site j; 0 otherwise
    IloArray<IloBoolVarArray> y(env);
    for (int i(0); i < n; i++) {
      IloBoolVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloBoolVar(env));
      y.add(tmp);
    }

    // distance between nodes
    IloArray<IloNumArray> d(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(arma::as_scalar(D(i, j)));
      d.add(tmp);
    }
    
    // objective function
    model.add(IloMinimize(env, w));

    // n constraints: all demand nodes must be allocated to exactly one facility
    for (int i(0); i < n; i++) {
      IloExpr e(IloSum(y[i]));
      IloRange c1(e == 1);
      model.add(c1);
    }
    
    // 1 constraint: locate exactly p facilities
    IloRange c2(IloSum(x) == p);
    model.add(c2);

    // n*n constraints: do not allocate a demand node to a not selected candidate site
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++) {
	IloConstraint c3(y[i][j] <= x[j]);
	model.add(c3);
      }

    // n contraints: distance between a node and the facility it is allocated to must be less than coverage distance
    for (int i(0); i < n; i++) {
      IloExpr e(IloScalProd(d[i], y[i]));
      IloConstraint c4(w >= e);
      model.add(c4);
    }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    // env.out() << "Solution status = " << cplex.getStatus() << endl;
    // env.out() << "Solution value = " << cplex.getObjValue() << endl;
    covDist = cplex.getObjValue();

    // retrieve center indices
    IloNumArray xVals(env);
    cplex.getValues(xVals, x);
    // env.out() << "x values = " << xVals << endl;

    for (int i(0); i < n; i++)
      if (IloRound(xVals[i]) == 1) {
	cIdx.push_back(i);
	partition.push_back(QList<int>());
      }

    // retrieve partition
    for (int i(0); i < n; i++) {
      IloNumArray yVals(env);
      cplex.getValues(yVals, y[i]);
      // env.out() << i << ": y values = " << yVals << endl;
      
      for (int j(0); j < p; j++)
	if (IloRound(yVals[cIdx[j]]) == 1)
	  partition[j].push_back(i);
    }
  }

  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unable to solve problem with " << p << " cluster(s)" << endl;
  }

  env.end();

  return covDist;
}

qreal Optimizer::pCenter_bis(int p, arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx, QList<QPair<int, int> > &ambiguous)
{
  qreal scaleFac(10000.0);

  // convert distance matrix to integer matrix
  arma::umat M = arma::conv_to<arma::umat>::from(D * scaleFac);

  int minDist = M.min();            // min distance in M matrix
  int maxDist = M.max();            // max distance in M matrix 
  arma::umat A;                     // coverage matrix
  int dist(-1);                     // current coverage distance
  int currP(0);                     // minimum number of centers

  while (minDist != maxDist) {

    partition.clear();
    cIdx.clear();

    dist = (minDist + maxDist) / 2;
    A = M <= dist;
    currP = setCovering(A, partition, cIdx);

    if (currP <= p)
      maxDist = dist;
    else
      minDist = dist + 1;
  }

  // retrieve optimal partition
  partition.clear();
  cIdx.clear();
  A = M <= minDist;
  setCovering(A, partition, cIdx);
  
  // identify instances allocated to multiple centers
  arma::umat V(1, cIdx.size());
  for (int i(0); i < cIdx.size(); i++)
    V(0, i) = cIdx[i];

  A = A.cols(V);

  for (arma::uword i(0); i < A.n_rows; i++) {
    if (arma::sum(A.row(i)) > 1) {
      arma::umat V1 = arma::find(A.row(i));
      for (arma::uword j(0); j < V1.n_rows; j++)
	ambiguous.push_back(QPair<int, int>(i, V(0, V1(j, 0))));
    }
  }

  return dist / scaleFac;
}


qreal Optimizer::pCenterOptP(arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx)
{
  qreal objValue(-1.0);                // objective value
  qreal prevObjValue(10.0);            // previous objective value
  qreal diff(0.0);                     // slop of the curve coverage/# of clusters
  qreal optDiff(0.0);
  int p(D.n_rows);
  int optP(p);

  while (--p > 1) {

    partition.clear();
    cIdx.clear();
    objValue = pCenter(p, D, partition, cIdx);

    diff = prevObjValue - objValue;
    
    if (diff < optDiff) {
      optP = p;
      optDiff = diff;
    }

    prevObjValue = objValue;
  }

  partition.clear();
  cIdx.clear();
  objValue = pCenter(optP, D, partition, cIdx);
   
  return objValue;
}

qreal Optimizer::pMedian(int p, arma::mat &D, QList<QList<int> > &partition, QList<int> &cIdx)
{
  int n(D.n_rows);                     // number of instances
  IloNum objValue(-1.0);               // objective value
  IloEnv env;

  try {
    IloModel model(env);

    // var x[j] = 1 if facility is located at candidate site j; 0 otherwise
    IloBoolVarArray x(env);
    for (int i(0); i < n; i++)
      x.add(IloBoolVar(env));

    // var y[i][j] = 1 if demand node i is allocated to facility located at candidate site j; 0 otherwise
    IloArray<IloNumVarArray> y(env);
    for (int i(0); i < n; i++) {
      IloNumVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloBoolVar(env, 0.0));
      y.add(tmp);
    }

    // distance between nodes
    IloArray<IloNumArray> d(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(arma::as_scalar(D(i, j)));
      d.add(tmp);
    }

    // objective function
    IloExpr obj(env);
    for (int i(0); i < n; i++) {
      IloExpr tmp(IloScalProd(d[i], y[i]));
      obj += tmp;
    }
      
    model.add(IloMinimize(env, obj));
      
    // n constraints: all demand node must be allocated to exactly one facility
    for (int i(0); i < n; i++) {
      IloExpr e(IloSum(y[i]));
      IloRange c1(e == 1);
      model.add(c1);
    }
    
    // 1 constraint: locate exactly p facilities
    IloRange c2(IloSum(x) == p);
    model.add(c2);

    // n*n constraints: do not allocate a demand node to a not selected candidate site
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++) {
	IloConstraint c3(y[i][j] <= x[j]);
	model.add(c3);
      }

    // n*(n-1) constraints: consecutivity of demand nodes allocated to the same site
    for (int i(1); i < n - 1; i++)
      for (int j(0); j < n; j++) {
	if (i != j) {
	  IloConstraint c4(y[i-1][j] + y[i+1][j] >= y[i][j]);
	  // model.add(c4);
	}
      }
    

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // retrieve center indices
    IloNumArray xVals(env);
    cplex.getValues(xVals, x);
    // env.out() << "x values = " << xVals << endl;

    cIdx.clear();
    for (int i(0); i < n; i++)
      if (IloRound(xVals[i]) == 1) {
	cIdx.push_back(i);
	partition.push_back(QList<int>());
      }

    // retrieve optimal partition
    for (int i(0); i < n; i++) {
      IloNumArray yVals(env);
      cplex.getValues(yVals, y[i]);
      // env.out() << i << ": y values = " << yVals << endl;
      
      bool found(false);
      int j(0);
      while (!found) {
	if (IloRound(yVals[cIdx[j]]) == 1) {
	  partition[j].push_back(i);
	  found = true;
	}
	j++;
      }
    }

    // solution features
    // env.out() << "Solution status = " << cplex.getStatus() << endl;
    // env.out() << "Solution value = " << cplex.getObjValue() << endl;
    objValue = cplex.getObjValue();
  }

  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return objValue / n;
}

qreal Optimizer::coClusterOptMatch(int p, arma::mat D, QList<QList<int> > &shotPartition, QList<int> &shotCIdx, arma::mat A, int pp, arma::mat DP, QList<QList<int> > &utterPartition, QList<int> &utterCIdx, QMap<int, QList<int> > &mapping, qreal lambda)
{
  int n(A.n_rows);                  // number of shots
  int m(A.n_cols);                  // number of utterances
  qreal objValue(-1.0);             // value of optimal solution

  IloEnv env;

  // normalize matrices
  A /= arma::sum(arma::sum(A));
  DP /= arma::max(arma::max(DP));
  // IloNum U = arma::sum(arma::sum(A));

  try {

    IloModel model(env);

    // distance between shots
    IloArray<IloNumArray> d(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(arma::as_scalar(D(i, j)));
      d.add(tmp);
    }
    
    // distance between utterances
    IloArray<IloNumArray> dp(env);
    for (int i(0); i < m; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < m; j++)
	tmp.add(arma::as_scalar(DP(i, j)));
      dp.add(tmp);
    }

    // overlapping time between utterance j and shot i
    IloArray<IloNumArray> a(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < m; j++)
	tmp.add(arma::as_scalar(A(i, j)));
      a.add(tmp);
    }

    // var pp: number of utterance clusters
    // IloIntVar pp(env, 1, p);

    // var x[j] = 1 if shot j is chosen as center cluster; 0 otherwise
    IloBoolVarArray x(env);
    for (int j(0); j < n; j++)
      x.add(IloBoolVar(env));

    // var y[i][j] = 1 if shot i is assigned to shot cluster center j; 0 otherwise
    IloArray<IloBoolVarArray> y(env);
    for (int i(0); i < n; i++) {
      IloBoolVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloBoolVar(env));
      y.add(tmp);
    }

    // var xp[j] = 1 if utterance j is chosen as center cluster; 0 otherwise
    IloBoolVarArray xp(env);
    for (int j(0); j < m; j++)
      xp.add(IloBoolVar(env));

    // var yp[i][j] = 1 if utterance i is assigned to utterance cluster center j; 0 otherwise
    IloArray<IloBoolVarArray> yp(env);
    for (int i(0); i < m; i++) {
      IloBoolVarArray tmp(env);
      for (int j(0); j < m; j++)
	tmp.add(IloBoolVar(env));
      yp.add(tmp);
    }

    // var z[i][j] = 1 if shot cluster of center i is assigned to utterance cluster of center j; 0 otherwise
    IloArray<IloBoolVarArray> z(env);
    for (int i(0); i < n; i++) {
      IloBoolVarArray tmp(env);
      for (int j(0); j < m; j++)
	tmp.add(IloBoolVar(env));
      z.add(tmp);
    }

    // var t[i][j][k][l] = z[i][j] * y[k][i] * yp[l][j]: additional variables to linearize (1) objective
    IloArray<IloArray<IloArray<IloNumVarArray> > > t(env);
    for (int i(0); i < n; i++) {
      IloArray<IloArray<IloNumVarArray> > tmp1(env);
      for (int j(0); j < m; j++) {
	IloArray<IloNumVarArray> tmp2(env);
	for (int k(0); k < n; k++) {
	  IloNumVarArray tmp3(env);
	  for (int l(0); l < m; l++)
	    if (a[k][l] > 0.0)
	      tmp3.add(IloNumVar(env, 0, 1));
	  if (tmp3.getSize() > 0)
	    tmp2.add(tmp3);
	}
	if (tmp2.getSize() > 0)
	  tmp1.add(tmp2);
      }
      if (tmp1.getSize() > 0)
	t.add(tmp1);
    }

    /*
    // var s[i][j][l] = yp[l][j] * sum_{k=1}^n y[k][i] * a[k][l]: additional variables to linearize (2) objective
    IloArray<IloArray<IloNumVarArray> > s(env);
    for (int i(0); i < n; i++) {
      IloArray<IloNumVarArray> tmp1(env);
      for (int j(0); j < m; j++) {
	IloNumVarArray tmp2(env);
	for (int l(0); l < m; l++)
	  tmp2.add(IloNumVar(env));
	tmp1.add(tmp2);
      }
      s.add(tmp1);
    }

    // var u[i][j] = z[i][j] * sum_{l=1}^m s[i][j][l]: additional variables to linearize (2) objective
    IloArray<IloNumVarArray> u(env);
    for (int i(0); i < n; i++) {
      IloNumVarArray tmp(env);
      for (int j(0); j < m; j++) {
	tmp.add(IloNumVar(env));
      }
      u.add(tmp);
    }
    */

    // objective function
    IloExpr shotClustObj(env);
    IloExpr utterClustObj(env);
    IloExpr matchObj(env);
    IloExpr obj(env);

    for (int i(0); i < n; i++)
      shotClustObj -= IloScalProd(d[i], y[i]);

    for (int i(0); i < m; i++)
      utterClustObj -= IloScalProd(dp[i], yp[i]);

    // linear expression (1) of matching objective
    for (int i(0); i < n; i++)
      for (int j(0); j < m; j++)
	for (int k(0), kp(0); k < n; k++) {
	  bool empty = true;
	  for (int l(0), lp(0); l < m; l++)
	    if (a[k][l] > 0.0) {
	      matchObj += t[i][j][kp][lp] * a[k][l];
	      lp++;
	      empty = false;
	    }
	  if (!empty)
	    kp++;
	}
    
    /*
    // quadratic expression of matching objective
    for (int i(0); i < n; i++)
      for (int j(0); j < m; j++) {
	IloExpr e(env);
	for (int l(0); l < m; l++)
	  e += s[i][j][l];
	matchObj += z[i][j] * e;
      }
    */

    /*
    // linear expression (2) of matching objective
    for (int i(0); i < n; i++)
      for (int j(0); j < m; j++)
	matchObj += u[i][j];
    */

    obj = lambda / 2.0 * (1.0 / n * shotClustObj + 1.0 / m * utterClustObj) + (1 - lambda) * matchObj;
    model.add(IloMaximize(env, obj));

    // n constraints: each shot has to be assigned to exactly one shot cluster center
    for (int i(0); i < n; i++) {
      IloExpr e(IloSum(y[i]));
      IloRange c1(e == 1);
      model.add(c1);
    }
    
    // 1 constraint: locate exactly p shot cluster centers
    IloRange c2(IloSum(x) == p);
    model.add(c2);

    // n*n constraints: no shot can be assigned to a shot not chosen as a cluster center
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++) {
	IloConstraint c3(y[i][j] <= x[j]);
	model.add(c3);
      }

    // n constraints: if a shot is chosen as center, assign it to itself
    for (int j(0); j < n; j++) {
      IloConstraint c4(x[j] <= y[j][j]);
      model.add(c4);
    }

    // (n-1) * n constraints: two consecutive shots can't be assigned to the same cluster
    for (int j(0); j < n; j++)
      for (int i(1); i < n; i++) {
	IloRange c5(y[i-1][j] + y[i][j] <= 1);
	model.add(c5);
      }
    
    // m constraints: each utterance has to be assigned to exactly one utterance cluster center
    for (int i(0); i < m; i++) {
      IloExpr e(IloSum(yp[i]));
      IloRange c6(e == 1);
      model.add(c6);
    }

    // 1 constraint: locate exactly pp utterance cluster centers
    IloConstraint c7(IloSum(xp) == pp);
    model.add(c7);

    // m constraints: if an utterance is chosen as center, assign it to itself
    for (int j(0); j < m; j++) {
      IloConstraint c8(xp[j] <= yp[j][j]);
      model.add(c8);
    }

    // m*m constraints: no utterance can be assigned to an utterance
    // not chosen as a cluster center
    for (int i(0); i < m; i++)
      for (int j(0); j < m; j++) {
	IloConstraint c9(yp[i][j] <= xp[j]);
	model.add(c9);
      }

    // m*n constraints: do not assign a shot cluster of center i to an
    // utterance not chosen as cluster center
    for (int i(0); i < n; i++)
      for (int j(0); j < m; j++) {
	IloConstraint c10(z[i][j] <= xp[j]);
	model.add(c10);
      }

    // n constraints: a shot cluster of center i must be allocated to
    // at most one utterance cluster of center j
    for (int i(0); i < n; i++) {
      IloConstraint c11(IloSum(z[i]) <= x[i]);
      model.add(c11);
    }

    // m constraints: an utterance cluster of center j must be
    // allocated to at least one shot cluster of center i
    for (int j(0); j < m; j++) {
      IloExpr e(env);
      for (int i(0); i < n; i++)
	e += z[i][j];
      IloConstraint c12(e >= xp[j]);
      model.add(c12);
    }

    // m*n*(n-1)/2*(n-1) constraints: do no assign overlapping shot
    // clusters to the same speaker
    for (int j(0); j < m; j++)
      for (int k(0); k < n-1; k++)
	for (int kp(k+1); kp < n; kp++) {
	  IloExpr e(z[k][j] + z[kp][j]);
	  for (int i(0); i < n - 1; i++) {
	    IloRange c13(e + y[i][k] + y[i+1][kp] <= 3);
	    model.add(c13);
	  }
	}

    // 3*n*n*m*m additional constraints to ensure that t[i][j][k][l] = z[i][j] * y[k][i] * yp[l][j]
    for (int i(0); i < n; i++)
      for (int j(0); j < m; j++)
	for (int k(0), kp(0); k < n; k++) {

	  bool empty(true);
	  
	  for (int l(0), lp(0); l < m; l++)
	    
	    if (a[k][l] > 0.0) {
	      IloConstraint c14(t[i][j][kp][lp] <= z[i][j]);
	      IloConstraint c15(t[i][j][kp][lp] <= y[k][i]);
	      IloConstraint c16(t[i][j][kp][lp] <= yp[l][j]);
	      model.add(c14);
	      model.add(c15);
	      model.add(c16);
	      lp++;
	      empty = false;
	    }
	  
	  if (!empty)
	    kp++;
	}

    /*
    // m*m + m*m*n additional constrains to ensure that s[i][j][l] = yp[l][j] * sum_{k=1}^n y[k][i] * a[k][l]
    for (int l(0); l < m; l++) {
     
      IloExpr e1(env);
      for (int k(0); k < n; k++)
	e1 += a[k][l];
      
      for (int i(0); i < n; i++)
	for (int j(0); j < m; j++) {
	  
	  IloConstraint c14(s[i][j][l] <= e1 * yp[l][j]);
	  model.add(c14);
	
	  IloExpr e2(env);
	  for (int k(0); k < n; k++) 
	    e2 += a[k][l] * y[k][i];
	  IloConstraint c15(s[i][j][l] <= e2);
	  model.add(c15);
	}
    }

    // 2*n*m additional constraints to ensure that u[i][j] = z[i][j] * sum_{l=1}^n s[i][j][l]
    for (int i(0); i < n; i++)
      for (int j(0); j < m; j++) {
	IloExpr e(env);
	
	for (int l(0); l < m; l++)
	  e += s[i][j][l];

	IloConstraint c16(u[i][j] <= U * z[i][j]);
	IloConstraint c17(u[i][j] <= e);

	model.add(c16);
	model.add(c17);
      }
    */

    // set some variables to constant values
    // set shot partition
    if (!shotCIdx.isEmpty()) {

      // looping over shot indices
      for (int i(0); i < n; i++) {
	
	// current index i possibly selected in center list
	int k(shotCIdx.indexOf(i));

	// index i has not been chosen as a cluster center
	if (k == -1) {
	  model.add(IloRange(x[i] == 0));
	  for (int j(0); j < n; j++)
	    model.add(IloRange(y[j][i] == 0));
	}

	// index i is a cluster center
	else {
	  model.add(IloRange(x[i] == 1));
	  for (int j(0); j < n; j++)
	    if (shotPartition[k].contains(j))
	      model.add(IloRange(y[j][i] == 1));
	    else
	      model.add(IloRange(y[j][i] == 0));
	}
      }
 
      shotCIdx.clear();
      shotPartition.clear();
    }

    // set utterance partition
    if (!utterCIdx.isEmpty()) {

      // looping over utterance indices
      for (int i(0); i < m; i++) {
	
	// current index i possibly selected in center list
	int k(utterCIdx.indexOf(i));

	// index i has not been chosen as a cluster center
	if (k == -1) {
	  model.add(IloRange(xp[i] == 0));
	  for (int j(0); j < m; j++)
	    model.add(IloRange(yp[j][i] == 0));
	}

	// index i is a cluster center
	else {
	  model.add(IloRange(xp[i] == 1));
	  for (int j(0); j < m; j++)
	    if (utterPartition[k].contains(j))
	      model.add(IloRange(yp[j][i] == 1));
	    else
	      model.add(IloRange(yp[j][i] == 0));
	}
      }
      
      utterCIdx.clear();
      utterPartition.clear();
    }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    // cplex.setParam(IloCplex::AggFill, 2);
    // cplex.setParam(IloCplex::PreInd, false);
    // cplex.setParam(IloCplex::TiLim, 5);
    // cplex.setParam(IloCplex::TreLim, 1000);
    cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    // env.out() << "Solution status = " << cplex.getStatus() << endl;
    // env.out() << "Solution value = " << cplex.getObjValue() << endl;
    objValue = cplex.getObjValue();

    // retrieve shot center indices
    IloNumArray xVals(env);
    cplex.getValues(xVals, x);
    // env.out() << "x values = " << xVals << endl;

    for (int i(0); i < n; i++)
      if (IloRound(xVals[i]) == 1) {
	shotCIdx.push_back(i);
	shotPartition.push_back(QList<int>());
      }

    arma::umat Y;
    Y.zeros(n, n);
    arma::umat YP;
    YP.zeros(m, m);
    arma::umat Z;
    Z.zeros(n, m);

    // retrieve shot partition
    for (int i(0); i < n; i++) {
      IloNumArray yVals(env);
      cplex.getValues(yVals, y[i]);
      // env.out() << i << ": y values = " << yVals << endl;
      
      for (int j(0); j < shotPartition.size(); j++)
	if (IloRound(yVals[shotCIdx[j]]) == 1) {
	  shotPartition[j].push_back(i);
	  Y(i, shotCIdx[j]) = 1;
	}
    }

    // retrieve utterance center indices
    IloNumArray xpVals(env);
    cplex.getValues(xpVals, xp);
    // env.out() << "xp values = " << xpVals << endl;

    for (int i(0); i < m; i++)
      if (IloRound(xpVals[i]) == 1) {
	utterCIdx.push_back(i);
	utterPartition.push_back(QList<int>());
      }

    // retrieve utterance partition
    for (int i(0); i < m; i++) {
      IloNumArray ypVals(env);
      cplex.getValues(ypVals, yp[i]);
      // env.out() << i << ": yp values = " << ypVals << endl;
      
      for (int j(0); j < utterPartition.size(); j++)
	if (IloRound(ypVals[utterCIdx[j]]) == 1) {
	  utterPartition[j].push_back(i);
	  YP(i, utterCIdx[j]) = 1;
	}
    }

    // retrieve mapping utterance/shot clusters
    for (int j(0); j < m; j++) {

      QList<int> tmp;
      
      for (int i(0); i < n; i++) {
      
	IloNum zVal = cplex.getValue(z[i][j]);

	if (IloRound(zVal) == 1) {
	  tmp.push_back(i);
	  Z(i, j) = 1;
	}

	// env.out() << i << " " << j << ": z value = " << IloRound(zVal) << endl;
      }
      
      if (!tmp.isEmpty())
	mapping[j] = tmp;
    }

    /*
    cout << A << endl;
    cout << arma::sum(arma::sum(A)) << endl;
    cout << Y << endl;
    cout << YP << endl;
    cout << Y.t() * A * YP << endl;
    cout << Z << endl;
    cout << (Y.t() * A * YP) % Z << endl;
    cout << arma::sum(arma::sum((Y.t() * A * YP) % Z)) << endl;
    */
  }
    
  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unable to solve problem with " << p << " cluster(s)" << endl;
  }

  env.end();

  return objValue;
}

qreal Optimizer::coClusterOptMatch_iter(int p, arma::mat D, QList<QList<int> > &shotPartition, QList<int> &shotCIdx, arma::mat A, int pp, arma::mat DP, QList<QList<int> > &utterPartition, QList<int> &utterCIdx, QMap<int, QList<int> > &mapping, qreal lambda)
{
  qreal obj(-1.0);
  qreal prevObj(-2.0);
  qreal epsilon(0.001);

  // initializing input partition
  coClusterOptMatch(p, D, shotPartition, shotCIdx, A, pp, DP, utterPartition, utterCIdx, mapping, 1.0);

  // looping until stop condition is reached
  do {

    utterPartition.clear();
    utterCIdx.clear();
    mapping.clear();
    obj = coClusterOptMatch(p, D, shotPartition, shotCIdx, A, pp, DP, utterPartition, utterCIdx, mapping, lambda);

    // qDebug() << obj - prevObj;

    shotPartition.clear();
    shotCIdx.clear();
    mapping.clear();
    obj = coClusterOptMatch(p, D, shotPartition, shotCIdx, A, pp, DP, utterPartition, utterCIdx, mapping, lambda);

    // qDebug() << "objective:" << obj;
    prevObj = obj;

  } while (obj - prevObj > epsilon);

  return obj;
}

qreal Optimizer::orderStorylines(QVector<int> &pos, const arma::mat &A, const QVector<int> &prevPos)
{
  // number of nodes (speakers)
  int n = A.n_rows;

  // maximum x coordinate
  int max(n - 1);

  // optimal value
  qreal objValue(-1.0);

  // permutation to return
  pos.resize(n);

  IloEnv env;

  try {
    IloModel model(env);
      
    // var x[i]: rank assigned to i-th speaker
    IloIntVarArray x(env);
    for (int i(0); i < n; i++)
      x.add(IloIntVar(env, 0, max));

    // coefficient a[i][j]: weight of edge linking node i and node j
    IloArray<IloNumArray> a(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++) {
	tmp.add(A(i, j));
      }
      a.add(tmp);
    }

    // d[i][j] = |x[i] - x[j]|: additional variables used to linearize objective
    IloArray<IloIntVarArray> d(env);
    for (int i(0); i < n; i++) {
      IloIntVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloIntVar(env, 0, max));
      d.add(tmp);
    }

    // objective function
    IloExpr obj(env);
    
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++)
	obj += a[i][j] * d[i][j];

    model.add(IloMinimize(env, obj));

    // linearization constraints
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++) {
	IloConstraint c1(d[i][j] >= x[i] - x[j]);
	IloConstraint c2(d[i][j] >= x[j] - x[i]);
	model.add(c1);
	model.add(c2);
      }
    
    // all different constraints
    for (int i(0); i < n - 1; i++)
      for (int j(i+1); j < n; j++) {
	IloConstraint c3(x[i] != x[j]);
	model.add(c3);
      }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    // cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    env.out() << "Solution status = " << cplex.getStatus() << endl;
    env.out() << "Solution value = " << cplex.getObjValue() << endl;
    objValue = cplex.getObjValue();

    // retrieve distances
    for (int i(0); i < n; i++) {
      IloNumArray dVals(env);
      cplex.getValues(dVals, d[i]);
      // cout << dVals << endl;
    }

    // retrieve optimal solution
    IloNumArray xVals(env);
    cplex.getValues(xVals, x);

    for (int i(0); i < n; i++)
      pos[i] = xVals[i];
  }

  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return objValue;
}

qreal Optimizer::orderStorylines_global(QVector<QVector<int> > &pos, const arma::mat &W, const QVector<QVector<int> > &prevPos, qreal alpha)
{
  // number of speakers
  int n = W.n_rows;

  // number of snapshots
  int T = W.n_cols / n;

  // maximum x coordinate
  int max(n - 1);

  // optimal value
  qreal objValue(-1.0);

  // sequence of permutation to return
  pos.resize(n);
  for (int i(0); i < n; i++)
    pos[i] = QVector<int>(T, -1);

  IloEnv env;

  try {
    IloModel model(env);
      
    // var x[i][t]: rank assigned to i-th speaker at scene t
    IloArray<IloIntVarArray> x(env);
    for (int i(0); i < n; i++) {
      IloIntVarArray tmp(env);
      for (int t(0); t < T; t++)
	tmp.add(IloIntVar(env, 0, max));
      x.add(tmp);
    }

    // coefficient a[i][j][t]: weight of edge linking node i and node j at scene t
    IloArray<IloArray<IloNumArray> > a(env);
    for (int i(0); i < n; i++) {
      IloArray<IloNumArray> tmp1(env);
      for (int j(0); j < n; j++) {
	IloNumArray tmp2(env);
	for (int t(0); t < T; t++) {
	  arma::mat A = W.submat(0, t * n, n - 1, (t + 1) * n - 1);
	  tmp2.add(A(i, j));
	}
	tmp1.add(tmp2);
      }
      a.add(tmp1);
    }

    // d[i][j][t] = |x[i][t] - x[j][t]|: additional variables used to linearize objective
    IloArray<IloArray<IloIntVarArray> > d(env);
    for (int i(0); i < n; i++) {
      IloArray<IloIntVarArray> tmp1(env);
      for (int j(0); j < n; j++) {
	IloIntVarArray tmp2(env);
	for (int t(0); t < T; t++)
	  tmp2.add(IloIntVar(env, 0, max));
	tmp1.add(tmp2);
      }
      d.add(tmp1);
    }

    // u[i][t] = |x[i][t] - x[i][t+1]|: additional variables used to linearize objective
    IloArray<IloIntVarArray> u(env);
    for (int i(0); i < n; i++) {
      IloIntVarArray tmp(env);
      for (int t(0); t < T - 1; t++)
	tmp.add(IloIntVar(env, 0, max));
      u.add(tmp);
    }

    // objective function
    IloExpr obj1(env);
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++)
	for (int t(0); t < T; t++)
	  obj1 += a[i][j][t] * d[i][j][t];

    IloExpr obj2(env);
    for (int i(0); i < n; i++)
      for (int t(0); t < T - 1; t++)
	obj2 += u[i][t];

    IloExpr obj = (1.0 - alpha) * obj1 + alpha * obj2;
    model.add(IloMinimize(env, obj));

    // linearization constraints
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++)
	for (int t(0); t < T; t++) {
	  IloConstraint c1(d[i][j][t] >= x[i][t] - x[j][t]);
	  IloConstraint c2(d[i][j][t] >= x[j][t] - x[i][t]);
	  model.add(c1);
	  model.add(c2);
	}

    for (int i(0); i < n; i++)
      for (int t(0); t < T - 1; t++) {
	IloConstraint c3(u[i][t] >= x[i][t] - x[i][t+1]);
	IloConstraint c4(u[i][t] >= x[i][t+1] - x[i][t]);
	model.add(c3);
	model.add(c4);
      }
    
    // all different at scene t constraints
    for (int t(0); t < T; t++)
      for (int i(0); i < n - 1; i++)
	for (int j(i+1); j < n; j++) {
	  IloConstraint c5(x[i][t] != x[j][t]);
	  model.add(c5);
	}

    // set x[i][t] variables
    for (int i(0); i < prevPos.size(); i++)
      for (int t(0); t < prevPos[i].size(); t++) {
	IloConstraint c6(x[i][t] == prevPos[i][t]);
	model.add(c6);
      }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    // cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    env.out() << "Solution status = " << cplex.getStatus() << endl;
    env.out() << "Solution value = " << cplex.getObjValue() << endl;
    objValue = cplex.getObjValue();
    
    // retrieve distances
    /*
    for (int i(0); i < n; i++) {
      for (int j(0); j < n; j++) {
	IloNumArray dVals(env);
	cplex.getValues(dVals, d[i][j]);
	for (int t(0); t < T; t++)
	  qDebug() << i << j << t << ":" << dVals[t] << endl;
      }
    }
    */

    // retrieve optimal solution
    for (int i(0); i < n; i++) {
      IloNumArray xVals(env);
      cplex.getValues(xVals, x[i]);
      for (int t(0); t < T; t++)
	pos[i][t] = xVals[t];
    } 
  }

  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return objValue;
}

qreal Optimizer::knapsack(const arma::vec &PV, const arma::vec &WV, const arma::mat &FR, const arma::mat &SD, qreal W, QList<int> &selected)
{
  qreal objValue(-1.0);
  int n(PV.n_rows);                  // number of variables

  IloEnv env;

  try {
    IloModel model(env);
      
    // var x[i] = 1 if object i is selected
    IloBoolVarArray x(env);
    for (int i(0); i < n; i++)
      x.add(IloBoolVar(env));

    // y[i][j] = x[i] * x[j]: additional variables for linearization purpose
    IloArray<IloNumVarArray> y(env);
    for (int i(0); i < n; i++) {
      IloNumVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloNumVar(env, 0.0, 1.0));
      y.add(tmp);
    }

    // cost vector
    IloNumArray p(env);
    for (int i(0); i < n; i++)
      p.add(PV(i));

    // weight vector
    IloNumArray w(env);
    for (int i(0); i < n; i++)
      w.add(WV(i));
      
    // formal redundancy matrix
    IloArray<IloNumArray> fr(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(as_scalar(FR(i, j)));
      fr.add(tmp);
    }

    // social dissimilarity matrix
    IloArray<IloNumArray> sd(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(as_scalar(SD(i, j)));
      sd.add(tmp);
    }

    // objective functions
    IloExpr obj(env);
    IloExpr obj1(env);
    IloExpr obj2(env);

    // relevance part
    obj1 = IloScalProd(p, x);

    // social dissimilarity part
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++)
	obj2 += sd[i][j] * y[i][j];

    obj = obj1 + obj2;
    model.add(IloMaximize(env, obj));

    // 1 constraint: sum of weight must not exceed knapsack capacity
    IloRange c1(IloScalProd(w, x) - W <= 0);
    model.add(c1);

    // 2*n*n linearization contraints
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++) {
	IloConstraint c3(y[i][j] <= x[i]);
	IloConstraint c4(y[i][j] <= x[j]);
	model.add(c3);
	model.add(c4);
      }

    // n*n constraints: no overlap between selected elements
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++) {
	IloRange c5(fr[i][j] * y[i][j] == 0.0);
	model.add(c5);
      }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    cplex.setOut(env.getNullStream());

    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    // env.out() << "Solution status = " << cplex.getStatus() << endl;
    // env.out() << "Solution value = " << cplex.getObjValue() << endl;

    objValue = cplex.getObjValue();

    // retrieve indices of selected objects
    IloNumArray xVals(env);
    cplex.getValues(xVals, x);

    // retrieve shot partition
    for (int i(0); i < n; i++)
      if (xVals[i] == 1)
	selected.push_back(i);
  }
  
  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return objValue;
}

qreal Optimizer::facilityLocation(const arma::vec &PV, const arma::vec &WV, const arma::mat &FR, const arma::mat &SR, qreal W, QList<int> &selected)
{
  qreal objValue(-1.0);
  int n(PV.n_rows);                  // number of variables

  IloEnv env;

  try {
    IloModel model(env);
      
    // var x[i] = 1 if object i is selected
    IloBoolVarArray x(env);
    for (int i(0); i < n; i++)
      x.add(IloBoolVar(env));

    // y[i][j] = 1 if demand node i is assigned to facility j
    IloArray<IloBoolVarArray> y(env);
    for (int i(0); i < n; i++) {
      IloBoolVarArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(IloBoolVar(env));
      y.add(tmp);
    }

    // cost vector
    IloNumArray p(env);
    for (int i(0); i < n; i++)
      p.add(PV(i));

    // weight vector
    IloNumArray w(env);
    for (int i(0); i < n; i++)
      w.add(WV(i));
      
    // formal redundancy matrix
    IloArray<IloNumArray> fr(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(as_scalar(FR(i, j)));
      fr.add(tmp);
    }

    // social redundancy matrix
    IloArray<IloNumArray> sr(env);
    for (int i(0); i < n; i++) {
      IloNumArray tmp(env);
      for (int j(0); j < n; j++)
	tmp.add(as_scalar(SR(i, j)));
      sr.add(tmp);
    }

    // objective functions
    IloExpr obj(env);
    IloExpr obj1(env);
    IloExpr obj2(env);
    IloExpr obj3(env);

    // relevance part
    obj1 = IloScalProd(p, x);

    // formal redundancy part
    for (int i(0); i < n - 1; i++)
      for (int j(i+1); j < n; j++)
	obj2 += fr[i][j] * y[i][j];

    // social redundancy part
    for (int i(0); i < n - 1; i++)
      for (int j(i+1); j < n; j++)
	obj3 += sr[i][j] * y[i][j];

    obj = obj1 + obj3;
    model.add(IloMaximize(env, obj));

    // 1 constraint: sum of weights must not exceed capacity
    IloRange c1(IloScalProd(w, x) - W <= 0);
    model.add(c1);

    // n constraints: all demand nodes must be allocated to exactly one facility
    for (int i(0); i < n; i++) {
      IloExpr e(IloSum(y[i]));
      IloRange c2(e == 1);
      model.add(c2);
    }

    // n*n constraints: do not allocate a demand node to a not selected candidate site
    for (int i(0); i < n; i++)
      for (int j(0); j < n; j++) {
	IloConstraint c3(y[i][j] <= x[j]);
	model.add(c3);
      }

    // solve model
    IloCplex cplex(model);

    // no output in terminal
    cplex.setOut(env.getNullStream());
    
    // solve model
    if (!cplex.solve()) {
      env.error() << "Failed to optimize LP." << endl;
      throw(-1);
    }

    // solution features
    env.out() << "Solution status = " << cplex.getStatus() << endl;
    env.out() << "Solution value = " << cplex.getObjValue() << endl;

    objValue = cplex.getObjValue();

    // retrieve indices of selected objects
    IloNumArray xVals(env);
    cplex.getValues(xVals, x);

    // retrieve shot partition
    for (int i(0); i < n; i++)
      if (xVals[i] == 1)
	selected.push_back(i);
  }
  
  catch (IloException &e) {
    cerr << "Concert exception caught: " << e << endl;
  }
    
  catch (...) {
    cerr << "Unknown exception caught: " << endl;
  }

  env.end();

  return objValue;
}

qreal Optimizer::mmr(arma::vec PV, const arma::vec &WV, const arma::mat &FR, const arma::mat &SD, qreal W, QList<int> &selected)
{
  arma::vec R;
  qreal objValue(0.0);;

  // (cost / weight) ratios
  QVector<QPair<qreal, int> > ratios;

  // flag
  bool sel(true);

  // qDebug();

  while (sel) {

    // set flag
    sel = false;

    // compute (cost / weight) ratios
    R = PV / WV;
    // R = PV;
    ratios = getRatios(R);

    // choose first feasible element not already selected
    int i(0);
    while (i < ratios.size() &&
	   (selected.contains(ratios[i].second) ||
	    !isFeasible(ratios[i].second, selected, WV, FR, W)))
      i++;

    if (i < ratios.size()) {

      int selIdx = ratios[i].second;

      // select element
      selected.push_back(selIdx);
      sel = true;

      // qDebug() << PV(selIdx) << WV(selIdx) << ratios[i];

      // update weight constraint
      W -= WV(selIdx);

      // update objective value
      objValue += PV(selIdx) + SD(selIdx, selIdx);

      // update cost vector
      for (uword j(0); j < PV.n_rows; j++)
	PV(j) = PV(j) + 2 * SD(j, selIdx);
    }
  }

  // qDebug();

  std::sort(selected.begin(), selected.end());

  return objValue;
}

void Optimizer::randomSelection(const arma::vec &WV, const arma::mat &FR, qreal W, QList<int> &selected)
{
  // initialite random number generator
  srand(time(0));
  
  QList<int> notSelected;
  for (uword i(0); i < WV.n_rows; i++)
    notSelected.push_back(i);

  // flag
  bool sel(true);

  while (notSelected.size() > 0) {
    
    // randomly choose index among not selected ones
    int selIdx = rand() % notSelected.size();

    // select element
    selected.push_back(notSelected[selIdx]);

    // update weight constraint
    W -= WV(notSelected[selIdx]);

    // remove selected element
    notSelected.removeAt(selIdx);

    // remove non-feasible elements
    for (int i(notSelected.size() - 1); i >= 0; i--)
      if (!isFeasible(notSelected[i], selected, WV, FR, W))
	notSelected.removeAt(i);
  }

  std::sort(selected.begin(), selected.end());
}

bool Optimizer::isFeasible(int idx, const QList<int> &selected, const arma::vec &WV, const arma::mat &FR, qreal W)
{
  for (int i(0); i < selected.size(); i++)
    if (FR(idx, selected[i]) > 0.0)
      return false;

  return WV(idx) <= W;
}

QVector<QPair<qreal, int> > Optimizer::getRatios(const arma::vec &R)
{
  QVector<QPair<qreal, int> > ratios(R.n_rows);

  for (uword i(0); i < R.n_rows; i++)
    ratios[i] = QPair<qreal, int>(R(i), i);
  
  std::sort(ratios.begin(), ratios.end());
  std::reverse(ratios.begin(), ratios.end());

  return ratios;
}

QVector<int> Optimizer::barycenterOrdering(const QStringList &speakers, const QMap<QString, QMap<QString, qreal> > &neighbors, const QVector<int> &initPos, int nIter, qreal refSpkWeight)
{
  QVector<int> pos(speakers.size());
  
  // set initial positions
  for (int i(0); i < speakers.size(); i++)
    if (initPos.isEmpty())
      pos[i] = i + 1;
    else
      pos[i] = initPos[i] + 1;

  for (int k(0); k < nIter; k++) {

    // compute barycenter of each speaker's neighborhood
    QMap<qreal, QStringList> neighborsBary;
    for (int i(0); i < speakers.size(); i++) {

      QString fSpk = speakers[i];
      qreal initWeight = refSpkWeight;
      qreal bary(initWeight * pos[speakers.indexOf(fSpk)]);
      qreal sumWeights(initWeight);
    
      for (int j(0); j < speakers.size(); j++)
	if (i != j) {

	  QString sSpk = speakers[j];
	  qreal weight = neighbors[fSpk][sSpk];
	  int currPos = pos[speakers.indexOf(sSpk)];

	  bary += weight * currPos;
	  sumWeights += weight;

	  // qDebug() << fSpk << sSpk << weight << currPos;
	}

      bary /= sumWeights;
      neighborsBary[bary].push_back(fSpk);
      // qDebug() << fSpk << bary;
    }
  
    // qDebug() << neighborsBary;

    // update positions
    QMap<qreal, QStringList>::const_iterator it = neighborsBary.begin();
  
    int i(1);
    while (it != neighborsBary.end()) {

      QStringList spks = it.value();

      for (int j(0); j < spks.size(); j++)
	pos[speakers.indexOf(spks[j])] = i++;

      it++;
    }

    // qDebug() << pos;
  }
  

  // decrement positions by one before returning them
  for (int i(0); i < pos.size(); i++)
    pos[i] -= 1;
  
  return pos;
}
