#include <QDebug>

#include "SpeechSegment.h"

SpeechSegment::SpeechSegment(qint64 start, qint64 end, const QString &text, const QString &speaker, Segment *parentSegment, Segment::Source source):
  Segment(start, parentSegment, source),
  m_end(end),
  m_speaker(speaker),
  m_text(text)
{
}

SpeechSegment::SpeechSegment(qint64 start, qint64 end, const QString &text, const QVector<QString> &speakers, const QVector<QStringList> &interLocs, Segment *parentSegment, Segment::Source source):
  Segment(start, parentSegment, source),
  m_end(end),
  m_speakers(speakers),
  m_interLocs(interLocs),
  m_text(text)
{
}

///////////////
// modifiers //
///////////////

void SpeechSegment::setEnd(qint64 end)
{
  m_end = end;
}

void SpeechSegment::setLabel(const QString &speaker, Segment::Source source)
{
  m_speakers[source] = speaker;
}

void SpeechSegment::setSpeaker(const QString &speaker, Segment::Source source)
{
  m_speakers[source] = speaker;
}

void SpeechSegment::setText(const QString &text)
{
  m_text = text;
}

void SpeechSegment::addInterLoc(const QString &speaker, SpkInteractDialog::InteractType type)
{
  if (!m_interLocs[type].contains(speaker)) {
    m_interLocs[type].push_back(speaker);
    qSort(m_interLocs[type]);
  }
}

void SpeechSegment::setInterLocs(const QStringList &interLocs, SpkInteractDialog::InteractType type)
{
  m_interLocs[type] = interLocs;
}

void SpeechSegment::resetInterLocs(SpkInteractDialog::InteractType type)
{
  m_interLocs[type].clear();
}

///////////////
// accessors //
///////////////

qint64 SpeechSegment::getEnd() const
{
  return m_end;
}

QString SpeechSegment::getText() const
{
  return m_text;
}

QString SpeechSegment::getLabel(Segment::Source source) const
{
  return m_speakers[source];
}

QVector<QString> SpeechSegment::getSpeakers() const
{
  return m_speakers;
}

QVector<QStringList> SpeechSegment::getInterLocs() const
{
  return m_interLocs;
}

QStringList SpeechSegment::getInterLocs(SpkInteractDialog::InteractType type) const
{
  return m_interLocs[type];
}

/////////////////////////////////////////
// reimplementation of abstract method //
/////////////////////////////////////////

QString SpeechSegment::display() const
{
  return "Speech segment " + QString::number(m_position);
}
