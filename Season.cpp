#include "Season.h"

Season::Season(Segment *parentSegment)
  : Segment(parentSegment)
{
}

Season::Season(int number, Segment *parentSegment, qint64 position, Segment::Source source)
  : Segment(position, parentSegment, source),
    m_number(number)
{
}

Season::~Season()
{
}

void Season::read(const QJsonObject &json)
{
  m_number = json["nb"].toInt();
  Segment::read(json);
}

void Season::write(QJsonObject &json) const
{
  json["nb"] = m_number;
  Segment::write(json);
}

QString Season::display() const
{
  return "Season " + QString::number(m_number);
}

int Season::getNumber() const
{
  return m_number;
}
