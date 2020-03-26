#ifndef SEASON_H
#define SEASON_H

#include <QString>

#include "Segment.h"

class Season: public Segment
{
 public:
  Season(Segment *parentSegment);
  Season(int number, Segment *parentSegment, qint64 position = 0, Segment::Source source = Segment::Manual);
  ~Season();
  void read(const QJsonObject &json);
  void write(QJsonObject &json) const;
  QString display() const;

  int getNumber() const;

 private:
  int m_number;
};

#endif
