#include <QString>

#include "Edge.h"

Edge::Edge(QVector3D v1, QPair<qreal, QPair<qreal, QVector3D> > v2, int v1Index, int v2Index, qreal weight, qreal colorWeight)
{
  m_v1 = v1;
  m_v2 = v2;
  m_v1Index = v1Index;
  m_v2Index = v2Index;
  m_weight = weight;
  m_colorWeight = colorWeight;
}

Edge::~Edge()
{
}

///////////////
// accessors //
///////////////

QVector3D Edge::getV1() const
{
  return m_v1;
}

QPair<qreal, QPair<qreal, QVector3D> > Edge::getV2() const
{
  return m_v2;
}

qreal Edge::getWeight() const
{
  return m_weight;
}

qreal Edge::getColorWeight() const
{
  return m_colorWeight;
}

int Edge::getV1Index() const
{
  return m_v1Index;
}

int Edge::getV2Index() const
{
  return m_v2Index;
}

///////////////
// modifiers //
///////////////

void Edge::setWeight(qreal weight)
{
  m_weight = weight;
}

void Edge::setColorWeight(qreal colorWeight)
{
  m_colorWeight = colorWeight;
}

void Edge::setV1(QVector3D v1)
{
  m_v1 = v1;
}

void Edge::setV2(QPair<qreal, QPair<qreal, QVector3D> > v2)
{
  m_v2 = v2;
}
