#include <QJsonArray>
#include <QDebug>

#include "VideoFrame.h"

VideoFrame::VideoFrame(Segment *parentSegment)
  : Segment(parentSegment)
{
}

VideoFrame::VideoFrame(int id, qint64 position, Segment *parentSegment, Segment::Source source, const QString &sub)
  : Segment(position, parentSegment, source),
    m_id(id),
    m_sub(sub)
{
  m_speaker = new QVector<QString>(3);
}

VideoFrame::~VideoFrame()
{
}

void VideoFrame::read(const QJsonObject &json)
{
  m_id = json["id"].toInt();
  m_sub = json["sub"].toString();

  m_speaker = new QVector<QString>(3);

  QJsonArray spkArray = json["spk"].toArray();
  for (int i(0); i < m_speaker->size(); i++) {
    QString speaker = spkArray[i].toString();
    while (speaker.indexOf(" ") == 0)
      speaker = speaker.replace(0, 1, "");
    m_speaker->replace(i, speaker);
  }

  QJsonArray facesArray = json["faces"].toArray();
  for (int i(0); i < facesArray.size(); i += 5) {
    QString label = facesArray[i].toString();
    QRect rect(facesArray[i+1].toInt(),
	       facesArray[i+2].toInt(),
	       facesArray[i+3].toInt(),
	       facesArray[i+4].toInt());
    m_faces.push_back(QPair<QString, QRect>(label, rect));
  }

  Segment::read(json);
}

void VideoFrame::write(QJsonObject &json) const
{
  json["id"] = m_id;
  json["sub"] = m_sub;

  QJsonArray spkArray;
  for (int i = 0; i < m_speaker->size(); i++)
    spkArray.append(m_speaker->at(i));

  json["spk"] = spkArray;

  QJsonArray facesArray;
  for (int i = 0; i < m_faces.size(); i++) {
    facesArray.append(m_faces[i].first);
    facesArray.append(m_faces[i].second.x());
    facesArray.append(m_faces[i].second.y());
    facesArray.append(m_faces[i].second.width());
    facesArray.append(m_faces[i].second.height());
  }

  json["faces"] = facesArray;

  Segment::write(json);
}

QString VideoFrame::display() const
{
  return "Frame " + QString::number(row() + 1);
}

//////////////
// acessors //
//////////////

int VideoFrame::getNumber() const
{
  return row() + 1;
}

QString VideoFrame::getSub() const
{
  return m_sub;
}

QString VideoFrame::getSpeaker(VideoFrame::SpeakerSource source) const
{
  return m_speaker->value(source);
}

int VideoFrame::getId() const
{
  return m_id;
}

QList<QPair<QString, QRect> > VideoFrame::getFaces() const
{
  return m_faces;
}

///////////////
// modifiers //
///////////////

void VideoFrame::setSub(const QString &sub)
{
  m_sub = sub;
}

void VideoFrame::setSpeaker(const QString &speaker, VideoFrame::SpeakerSource source)
{
  m_speaker->replace(source, speaker);
}

void VideoFrame::clearSpeaker(VideoFrame::SpeakerSource source)
{
  m_speaker->replace(source, "");
}

void VideoFrame::setFaces(const QList<QPair<QString, QRect> > &faces)
{
  m_faces = faces;
}

void VideoFrame::clearFaces()
{
  m_faces = QList<QPair<QString, QRect> >();
}
