#ifndef SERIES_H
#define SERIES_H

#include <QString>

#include "Segment.h"

class Series: public Segment
{
 public:
  Series(qint64 position = 0, Segment *parentSegment = 0, Segment::Source source = Segment::Manual);
  ~Series();
  void read(const QJsonObject &json);
  void write(QJsonObject &json) const;
  QString display() const;

  QString getName() const;
  void setName(const QString &name);

 private:
  QString m_name;
};

#endif
