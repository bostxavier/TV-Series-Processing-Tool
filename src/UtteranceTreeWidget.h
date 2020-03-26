#ifndef UTTERANCETREEWIDGET_H
#define UTTERANCETREEWIDGET_H

#include <QWidget>
#include <QRectF>
#include <QMap>

#include <armadillo>

#include "UtteranceTree.h"

class UtteranceTreeWidget: public QWidget
{
  Q_OBJECT
  
 public:
  UtteranceTreeWidget(int width = 480, int height = 270, QWidget *parent = 0);
  QList<QList<int>> getPartition();

  public slots:
    void updateUtteranceTree(const arma::mat &S, const arma::mat &W, const arma::umat &map, const arma::mat &SigmaInv);
    void setDistance(UtteranceTree::DistType dist);
    void setCovInv(const arma::mat &CovInv);
    void setAgrCrit(UtteranceTree::AgrCrit agr);
    void setPartitionMethod(UtteranceTree::PartMeth partMeth);
    void setWeight(const arma::mat &W);
    void setCurrSubtitle(int subIdx);
    void normalizeVectors(bool checked);
    void setDiff(const arma::mat &Diff);
    void releasePos(bool released);
    
 signals:
    void playSubtitle(QList<int> utter);
    void setSpeakers(QList<int> spkIdx);
    
 protected:
  void paintEvent(QPaintEvent * event);
  void mousePressEvent(QMouseEvent * event);
  void mouseDoubleClickEvent(QMouseEvent * event);
  void mouseMoveEvent(QMouseEvent * event);

 private:
  int m_width;
  int m_height;
  arma::mat m_S;
  arma::mat m_unnormS;
  arma::mat m_W;
  arma::mat m_SigmaInv;
  arma::umat m_map;
  UtteranceTree *m_tree;
  QString m_currSub;
  QVector<qreal> m_cutDist;
  QList<QList<int>> m_partition;
  qreal m_yPad;
  qreal m_YRatio;
  QPoint m_mousePos;
  QList<int> m_selectedUtter;
  bool m_posReleased;
};

#endif
