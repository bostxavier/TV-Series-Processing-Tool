#ifndef SHOT_H
#define SHOT_H

#include <QMap>

#include "Segment.h"
#include "VideoFrame.h"
#include "Face.h"

class Shot: public Segment
{
 public:
  enum TransitionType {
    None, FadeOut, FadeIn, FadeOutIn, Dissolve, Cut
  };

  Shot(Segment *parentSegment);
  Shot(qint64 position, TransitionType transitionType, Segment *parentSegment, Segment::Source source = Segment::Manual);
  ~Shot();
  void read(const QJsonObject &json);
  void write(QJsonObject &json) const;
  QString display() const;
  int getNumber() const;
  void setEnd(qint64 end);
  void setCamera(int camera, Segment::Source source);
  void clearFaces();
  void clearMusicRates();
  void appendFaces(qint64 position, const QList<QRect> &faces);
  void appendMusicRate(QPair<qint64, qreal> musicRate);
  qint64 getEnd() const;
  int getCamera(Segment::Source source) const;
  QString getLabel(Segment::Source source) const;
  QMap<QString, int> getSpeakerList(VideoFrame::SpeakerSource source);
  QList<QPair<qint64, QList<Face> > > getFaces() const;
  QList<QPair<qint64, qreal> > getMusicRates() const;
  qreal computeHeightRatio();

 private:
  TransitionType m_transitionType;
  qint64 m_end;
  QVector<int> m_camera;
  QList<QPair<qint64, QList<Face> > > m_faces;
  QList<QPair<qint64, qreal> > m_musicRates;
};

#endif
