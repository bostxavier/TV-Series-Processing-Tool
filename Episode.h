#ifndef EPISODE_H
#define EPISODE_H

#include <QString>
#include <QSize>

#include "Segment.h"
#include "SpeechSegment.h"

class Episode: public Segment
{
 public:
  Episode(Segment *parentSegment);
  Episode(int number, const QString &fName, Segment *parentSegment, const QString &name = QString(), qint64 position = 0, Segment::Source source = Segment::Manual);
  ~Episode();
  void read(const QJsonObject &json);
  void write(QJsonObject &json) const;
  QString display() const;
  void setResolution(const QSize &resolution);
  void setFps(qreal fps);
  void setSpeechSegments(QList<SpeechSegment *> &speechSegments);
  QSize getResolution() const;
  qreal getFps() const;
  QString getFName() const;
  int getNumber() const;
  QList<SpeechSegment *> getSpeechSegments() const;
  
 private:
  int m_number;
  QString m_name;
  QString m_fName;
  QSize m_resolution;
  qreal m_fps;
  QList<SpeechSegment *> m_speechSegments;
};

#endif
