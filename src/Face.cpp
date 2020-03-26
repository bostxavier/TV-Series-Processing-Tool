#include <QDebug>

#include "Face.h"

Face::Face(const QRect &bbox, const QList<QPoint> &landmarks, int id, const QString &name):
  m_bbox(bbox),
  m_landmarks(landmarks),
  m_id(id),
  m_name(name)
{
}

QRect Face::getBbox() const
{
  return m_bbox;
}

QList<QPoint> Face::getLandmarks() const
{
  return m_landmarks;
}

int Face::getId() const
{
  return m_id;
}

QString Face::getName() const
{
  return m_name;
}
