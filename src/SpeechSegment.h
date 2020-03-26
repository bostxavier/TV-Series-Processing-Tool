#ifndef SPEECHSEGMENT_H
#define SPEECHSEGMENT_H

#include <QString>

#include "Segment.h"
#include "VideoFrame.h"
#include "SpkInteractDialog.h"

class SpeechSegment: public Segment
{
 public:
  SpeechSegment(qint64 start, qint64 end, const QString &text, const QString &speaker, Segment *parentSegment = 0, Segment::Source source = Segment::Manual);
  SpeechSegment(qint64 start, qint64 end, const QString &text, const QVector<QString> &speakers, const QVector<QStringList> &interLocs, Segment *parentSegment = 0, Segment::Source source = Segment::Manual);

  void setEnd(qint64 end);
  void setLabel(const QString &speaker, Segment::Source source);
  void setSpeaker(const QString &speaker, Segment::Source source = Segment::Manual);
  void setText(const QString &text);
  void addInterLoc(const QString &speaker, SpkInteractDialog::InteractType type);
  void setInterLocs(const QStringList &interLocs, SpkInteractDialog::InteractType type);
  void resetInterLocs(SpkInteractDialog::InteractType type);

  qint64 getEnd() const;
  QString getLabel(Segment::Source source) const;
  QString getText() const;
  QVector<QString> getSpeakers() const;
  QVector<QStringList> getInterLocs() const;
  QStringList getInterLocs(SpkInteractDialog::InteractType type) const;
  
  QString display() const;

 private:
  qint64 m_end;
  QVector<QString> m_speakers;
  QVector<QStringList> m_interLocs;
  QString m_speaker;
  QString m_text;
};

#endif
