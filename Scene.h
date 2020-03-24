#ifndef SCENE_H
#define SCENE_H

#include <QString>

#include "Segment.h"

class Scene: public Segment
{
 public:
  Scene(Segment *parentSegment);
  Scene(qint64 position, Segment *parentSegment, Segment::Source source = Segment::Manual);
  ~Scene();
  void read(const QJsonObject &json);
  void write(QJsonObject &json) const;
  QString display() const;
  int getNumber() const;

 private:
  bool m_manual;
  QString m_name;
};

#endif
