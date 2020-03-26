#include <QJsonArray>

#include "Episode.h"
#include "Shot.h"
#include "VideoFrame.h"

#include <QDebug>

Shot::Shot(Segment *parentSegment)
  : Segment(parentSegment)
{
}

Shot::Shot(qint64 position, TransitionType transitionType, Segment *parentSegment, Segment::Source source)
  : Segment(position, parentSegment, source),
    m_transitionType(transitionType),
    m_end(-1)
{
  m_camera.resize(2);
  m_camera[Segment::Manual] = -1;
  m_camera[Segment::Automatic] = -1;
}

Shot::~Shot()
{
}

void Shot::read(const QJsonObject &json)
{
  m_source = static_cast<Source>(json["source"].toInt());
  m_transitionType = static_cast<TransitionType>(json["trans"].toInt());
  m_camera.resize(2);

  QJsonArray camArray = json["cam"].toArray();
  for (int i(0); i < m_camera.size(); i++)
    m_camera[i] = camArray[i].toInt();

  Segment::read(json);

  // creating frames contained in current shot
  m_end = json["end"].toInt();

  // retrieving face boundaries
  QJsonArray facesArray = json["faces"].toArray();

  for (int i(0); i < facesArray.size(); i++) {

    QJsonArray pairArray = facesArray[i].toArray();

    qint64 position = pairArray[0].toInt();
    QJsonArray faces = pairArray[1].toArray();
    
    QList<Face> faceList;
    
    for (int j(0); j < faces.size(); j++) {
      QJsonArray faceFeatures = faces[j].toArray();

      // bounding box
      int x = faceFeatures[0].toInt();
      int y = faceFeatures[1].toInt();
      int width = faceFeatures[2].toInt();
      int height = faceFeatures[3].toInt();
      QRect bbox(x, y, width, height);
      
      QList<QPoint> landmarks;
      int faceId = -1;
      QString name = "";
      
      // face landmarks and id
      if (faceFeatures.size() > 4) {
	faceId = faceFeatures[4].toInt();
	name = faceFeatures[5].toString();

	if (faceFeatures.size() > 6) {
	  QPoint eyeLeft(faceFeatures[6].toInt(), faceFeatures[7].toInt());
	  QPoint eyeRight(faceFeatures[8].toInt(), faceFeatures[9].toInt());
	  QPoint mouthLeft(faceFeatures[10].toInt(), faceFeatures[11].toInt());
	  QPoint mouthRight(faceFeatures[12].toInt(), faceFeatures[13].toInt());
	  QPoint nose(faceFeatures[14].toInt(), faceFeatures[15].toInt());
	  landmarks.push_back(eyeLeft);
	  landmarks.push_back(eyeRight);
	  landmarks.push_back(mouthLeft);
	  landmarks.push_back(mouthRight);
	  landmarks.push_back(nose);
	}
      }
      faceList.push_back(Face(bbox, landmarks, faceId, name));
    }
    m_faces.push_back(QPair<qint64, QList<Face> >(position, faceList));
  }

  // retrieving musical features
  QJsonArray musicArray = json["music"].toArray();

  for (int i(0); i < musicArray.size(); i++) {

    QJsonArray pairArray = musicArray[i].toArray();

    qint64 position = pairArray[0].toInt();
    qreal value = pairArray[1].toDouble();

    m_musicRates.push_back(QPair<qint64, qreal>(position, value));
  }
}

void Shot::write(QJsonObject &json) const
{
  json["end"] = m_end;
  json["source"] = m_source;
  json["trans"] = m_transitionType;

  QJsonArray camArray;
  for (int i = 0; i < m_camera.size(); i++)
    camArray.append(m_camera[i]);

  json["cam"] = camArray;

  QJsonArray facesArray;
  for (int i(0); i < m_faces.size(); i++) {
    QJsonArray pairArray;

    // faces position
    pairArray.append(m_faces[i].first);
    
    // retrieving faces boundaries
    QList<Face> faceFeatures = m_faces[i].second;
    QJsonArray featList;
    for (int j(0); j < faceFeatures.size(); j++) {
      QJsonArray currFeat;
      currFeat.append(faceFeatures[j].getBbox().x());
      currFeat.append(faceFeatures[j].getBbox().y());
      currFeat.append(faceFeatures[j].getBbox().width());
      currFeat.append(faceFeatures[j].getBbox().height());

      currFeat.append(faceFeatures[j].getId());
      currFeat.append(faceFeatures[j].getName());
      
      QList<QPoint> landmarks = faceFeatures[j].getLandmarks();
      for (int k(0); k < landmarks.size(); k++) {
	currFeat.append(landmarks[k].x());
	currFeat.append(landmarks[k].y());
      }
      
      featList.append(currFeat);
    }
    pairArray.append(featList);
    facesArray.append(pairArray);
  }

  json["faces"] = facesArray;

  QJsonArray musicArray;
  for (int i(0); i < m_musicRates.size(); i++) {
    QJsonArray pairArray;

    // musical feature position
    pairArray.append(m_musicRates[i].first);

    // musical feature value
    pairArray.append(m_musicRates[i].second);

    // add pair to current array
    musicArray.append(pairArray);
  }

  json["music"] = musicArray;

  Segment::write(json);
}

QString Shot::display() const
{
  return "Shot " + QString::number(row()+1);
}

///////////////
// modifiers //
///////////////


void Shot::setCamera(int camera, Segment::Source source)
{
  m_camera[source] = camera;
}

void Shot::setEnd(qint64 end)
{
  m_end = end;
}

void Shot::clearFaces()
{
  m_faces.clear();
}

void Shot::clearMusicRates()
{
  m_musicRates.clear();
}

void Shot::appendFaces(qint64 position, const QList<QRect> &faces)
{
  QList<Face> newFaces;
  for (int i(0); i < faces.size(); i++) {
    Face newFace(faces[i], QList<QPoint>(), -1, "");
    newFaces.push_back(newFace);
  }
  m_faces.push_back(QPair<qint64, QList<Face> >(position, newFaces));
}

void Shot::appendMusicRate(QPair<qint64, qreal> musicRate)
{
  m_musicRates.push_back(musicRate);
}

///////////////
// accessors //
///////////////

int Shot::getNumber() const
{
  return row() + 1;
}

qint64 Shot::getEnd() const
{
  return m_end;
}

int Shot::getCamera(Segment::Source source) const
{
  return m_camera[source];
}

QString Shot::getLabel(Segment::Source source) const
{
  return "C" + QString::number(getCamera(source));
}

QMap<QString, int> Shot::getSpeakerList(VideoFrame::SpeakerSource source)
{
  QMap<QString, int> speakerList;
  QString speakerLabel;
  VideoFrame *frame;
  int n;
  
  for (int i(0); i < childCount(); i++) {
    speakerLabel = (frame = dynamic_cast<VideoFrame *>(child(i)))->getSpeaker(source);
    
    if (!speakerLabel.isEmpty()) {
      n = speakerList.value(speakerLabel);
      speakerList.insert(speakerLabel, ++n);
    }
  }

  return speakerList;
}

QList<QPair<qint64, QList<Face> > > Shot::getFaces() const
{
  return m_faces;
}

QList<QPair<qint64, qreal> > Shot::getMusicRates() const
{
  return m_musicRates;
}

qreal Shot::computeHeightRatio()
{
  qreal ratio(0.0);
  QList<int> faceHeights;

  // retrieving video height
  int videoHeight(0);
  Episode *episode = dynamic_cast<Episode *>(parent()->parent());
  videoHeight = episode->getResolution().height();
  
  // looping over frames
  for (int i(0); i < m_faces.size(); i++) {
    
    // retrieve maximum face height among faces detected on current frame
    int max(0);

    QList<Face> currFaces = m_faces[i].second;
    for (int j(0); j < currFaces.size(); j++) {
      QRect bbox = currFaces[j].getBbox();
      if (bbox.height() > max)
	max = bbox.height();
    }

    faceHeights.push_back(max);
  }

  // retrieve median face height
  qSort(faceHeights);

  if (faceHeights.size() > 0) {
    int lim = faceHeights.size() / 2;

    int i(0);
    while (i <= lim) {
      ratio = faceHeights[i];
      i++;
    }
  }

  return ratio / videoHeight;
}
