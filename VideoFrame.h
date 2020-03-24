#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include <QString>
#include <QVector>
#include <QRect>
#include <QPair>

#include "Segment.h"

class VideoFrame: public Segment
{
 public:
  enum SpeakerSource {
    Ref, Hyp1, Hyp2
  };

  VideoFrame(Segment *parentSegment);
  VideoFrame(int id, qint64 position, Segment *parentSegment, Segment::Source source = Segment::Manual, const QString &sub = QString());
  ~VideoFrame();
  void read(const QJsonObject &json);
  void write(QJsonObject &json) const;
  QString display() const;
  int getNumber() const;
  QString getSub() const;
  QString getSpeaker(VideoFrame::SpeakerSource source) const;
  QList<QPair<QString, QRect> > getFaces() const;
  int getId() const;
  void setSub(const QString &sub);
  void setSpeaker(const QString &speaker, VideoFrame::SpeakerSource source);
  void clearSpeaker(VideoFrame::SpeakerSource source);
  void setFaces(const QList<QPair<QString, QRect> > &faces);
  void clearFaces();

 private:
  int m_id;
  QString m_sub;
  QVector<QString> *m_speaker;
  QList<QPair<QString, QRect> > m_faces;
};

#endif

