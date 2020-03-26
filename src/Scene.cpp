#include "Scene.h"

Scene::Scene(Segment *parentSegment)
  : Segment(parentSegment)
{
}

Scene::Scene(qint64 position, Segment *parentSegment, Segment::Source source)
  : Segment(position, parentSegment, source)
{
}

Scene::~Scene()
{
}

void Scene::read(const QJsonObject &json)
{
  m_source = static_cast<Source>(json["source"].toInt());
  Segment::read(json);
}

void Scene::write(QJsonObject &json) const
{
  json["source"] = m_source;
  Segment::write(json);
}

QString Scene::display() const
{
  return "Scene " + QString::number(row()+1);
}

int Scene::getNumber() const
{
  return row() + 1;
}
