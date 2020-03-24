#ifndef VERTEX_H
#define VERTEX_H

#include <QVector3D>
#include <QString>

class Vertex
{
 public:
  Vertex(QVector3D v, const QString &label, qreal weight = 0.0, qreal colorWeight = 1.0);
  ~Vertex();

  QVector3D getV() const;
  QString getLabel() const;
  qreal getWeight() const;
  qreal getColorWeight() const;
  int getCom() const;

  void setV(QVector3D v);
  void setWeight(qreal weight);
  void setColorWeight(qreal colorWeight);
  void setCom(int com);

 protected:

 private:
  QVector3D m_v;
  QString m_label;
  qreal m_weight;
  qreal m_colorWeight;
  int m_com;
};

bool operator<(const Vertex &v1, const Vertex &v2);

#endif
