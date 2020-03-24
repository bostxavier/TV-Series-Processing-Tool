#include "Series.h"

Series::Series(qint64 position , Segment *parentSegment, Segment::Source source)
  : Segment(position, parentSegment, source)
{
}

Series::~Series()
{
}

void Series::read(const QJsonObject &json)
{
  m_name = json["name"].toString();
  Segment::read(json);
}

void Series::write(QJsonObject &json) const
{
  json["name"] = m_name;
  return Segment::write(json);
}

QString Series::display() const
{
  return m_name;
}

QString Series::getName() const
{
  return m_name;
}

void Series::setName(const QString &name)
{
  m_name = name;
}
