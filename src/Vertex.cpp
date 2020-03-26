#include "Vertex.h"

Vertex::Vertex(QVector3D v, const QString &label, qreal weight, qreal colorWeight)
{
  m_v = v;
  m_label = label;
  m_weight = weight;
  m_colorWeight = colorWeight;
  m_com = -1;
}

Vertex::~Vertex()
{
}

///////////////
// accessors //
///////////////

QVector3D Vertex::getV() const
{
  return m_v;
}

QString Vertex::getLabel() const
{
  return m_label;
}

qreal Vertex::getWeight() const
{
  return m_weight;
}

qreal Vertex::getColorWeight() const
{
  return m_weight / 5.0;
}

int Vertex::getCom() const
{
  return m_com;
}

///////////////
// modifiers //
///////////////

void Vertex::setV(QVector3D v)
{
  m_v = v;
}

void Vertex::setWeight(qreal weight)
{
  m_weight = weight;
}
 
void Vertex::setColorWeight(qreal colorWeight)
{
  m_colorWeight = colorWeight;
}

void Vertex::setCom(int com)
{
  m_com = com;
}

bool operator<(const Vertex &v1, const Vertex &v2)
{
  return v1.getWeight() < v2.getWeight();
}
