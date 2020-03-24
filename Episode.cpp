#include <QJsonArray>
#include <QDebug>

#include "Episode.h"

Episode::Episode(Segment *parentSegment)
  : Segment(parentSegment)
{
}

Episode::Episode(int number, const QString &fName, Segment *parentSegment, const QString &name, qint64 position, Segment::Source source)
  : Segment(position, parentSegment, source),
    m_number(number),
    m_name(name),
    m_fName(fName),
    m_resolution(QSize())
{
}

Episode::~Episode()
{
}

void Episode::read(const QJsonObject &json)
{
  // regular expression to detext spaces at beginning/end of speaker label
  QRegularExpression spaces("^\\s+.+\\s+$");
  
  m_number = json["nb"].toInt();
  m_name = json["name"].toString();
  m_fName = json["fName"].toString();
  m_resolution.setWidth(json["width"].toInt());
  m_resolution.setHeight(json["height"].toInt());
  m_fps = json["fps"].toDouble();
  Segment::read(json);

  QJsonArray dataArray = json["data"].toArray();

  QJsonObject videoData = dataArray[0].toObject();
  m_fName = videoData["fName"].toString(); 
  m_resolution.setWidth(videoData["width"].toInt());
  m_resolution.setHeight(videoData["height"].toInt());
  m_fps = videoData["fps"].toDouble();
  Segment::read(videoData);

  QJsonObject audioData = dataArray[1].toObject();
  QJsonArray utterArray = audioData["subtitles"].toArray();
  for (int i(0); i < utterArray.size(); i++) {
    QJsonObject utterance = utterArray[i].toObject();

    QJsonArray spkArray= utterance["spk"].toArray();
    QVector<QString> speakers;
    for (int j(0); j < spkArray.size(); j++) {
      QString spk = spkArray[j].toString();
      speakers.push_back(spk);
    }
    
    QJsonArray interLocArray = utterance["inter"].toArray();
    QVector<QStringList> interLoc;
    for (int j(0); j < interLocArray.size(); j++) {
      QJsonArray srcInterLocArray = interLocArray[j].toArray();
      QStringList srcInterLoc;
      
      for (int k(0); k < srcInterLocArray.size(); k++) {
	QString spk = srcInterLocArray[k].toString();
	srcInterLoc.push_back(spk);
      }

      interLoc.push_back(srcInterLoc);
    }

    m_speechSegments.push_back(new SpeechSegment(utterance["start"].toInt(),
						 utterance["end"].toInt(),
						 utterance["text"].toString(),
						 speakers,
						 interLoc,
						 this));
  }
}

void Episode::write(QJsonObject &json) const
{
  json["nb"] = m_number;
  json["name"] = m_name;

  QJsonArray dataArray;

  QJsonObject videoData;
  videoData["fName"] = m_fName;
  videoData["width"] = m_resolution.width();
  videoData["height"] = m_resolution.height();
  videoData["fps"] = m_fps;
  Segment::write(videoData);

  QJsonObject audioData;
  QJsonArray utterArray;
  for (int i(0); i < m_speechSegments.size(); i++) {
    QJsonObject utterance;
    utterance["start"] = m_speechSegments[i]->getPosition();
    utterance["end"] = m_speechSegments[i]->getEnd();
    utterance["text"] = m_speechSegments[i]->getText();

    QJsonArray spkArray;
    QVector<QString> speakers = m_speechSegments[i]->getSpeakers();

    for (int i(0); i < speakers.size(); i++)
      spkArray.append(speakers[i]);

    utterance["spk"] = spkArray;

    QJsonArray interLocArray;
    QVector<QStringList> interLoc = m_speechSegments[i]->getInterLocs();

    for (int i(0); i < interLoc.size(); i++) {
      QJsonArray srcInterLocArray;
      for (int j(0); j < interLoc[i].size(); j++)
	srcInterLocArray.append(interLoc[i][j]);
      interLocArray.append(srcInterLocArray);
    }
    
    utterance["inter"] = interLocArray;
    
    utterArray.append(utterance);
  }
  
  audioData["subtitles"] = utterArray;

  dataArray.append(videoData);
  dataArray.append(audioData);

  json["data"] = dataArray;
}

QString Episode::display() const
{
  return "Episode " + QString::number(m_number);
}

void Episode::setResolution(const QSize &resolution)
{
  m_resolution = resolution;
}

void Episode::setFps(qreal fps)
{
  m_fps = fps;
}

void Episode::setSpeechSegments(QList<SpeechSegment *> &speechSegments)
{
  m_speechSegments = speechSegments;
}

QSize Episode::getResolution() const
{
  return m_resolution;
}

qreal Episode::getFps() const
{
  return m_fps;
}

QString Episode::getFName() const
{
  return m_fName;
}

int Episode::getNumber() const
{
  return m_number;
}

QList<SpeechSegment *> Episode::getSpeechSegments() const
{
  return m_speechSegments;
}
