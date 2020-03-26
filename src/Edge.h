#ifndef EDGE_H
#define EDGE_H

#include <QVector3D>
#include <QPair>

class Edge
{
 public:
  Edge(QVector3D v1, QPair<qreal, QPair<qreal, QVector3D> > v2, int v1Index, int v2Index, qreal weight = 0.0, qreal colorWeight = 1.0);
  ~Edge();

  QVector3D getV1() const;
  QPair<qreal, QPair<qreal, QVector3D> > getV2() const;
  qreal getWeight() const;
  qreal getColorWeight() const;
  int getV1Index() const;
  int getV2Index() const;

  void setWeight(qreal weight);
  void setColorWeight(qreal colorWeight);
  void setV1(QVector3D v1);
  void setV2(QPair<qreal, QPair<qreal, QVector3D> > v2);
  
 protected:

 private:
  QVector3D m_v1;
  QPair<qreal, QPair<qreal, QVector3D> > m_v2;
  int m_v1Index;
  int m_v2Index;
  qreal m_weight;
  qreal m_colorWeight;
};

#endif
