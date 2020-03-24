#include <QTime>
#include <QJsonArray>
#include <QStringList>
#include <QDebug>

#include "Segment.h"
#include "Season.h"
#include "Episode.h"
#include "Scene.h"
#include "Shot.h"

Segment::Segment(Segment *parentSegment)
  : m_parentSegment(parentSegment)
{
}

Segment::Segment(qint64 position, Segment *parentSegment, Source source)
  : m_position(position),
    m_source(source),
    m_parentSegment(parentSegment)
{
}

Segment::~Segment()
{
  qDeleteAll(m_childSegments);
}

void Segment::read(const QJsonObject &json)
{
  m_position = json["pos"].toInt();

  m_source = static_cast<Source>(json["source"].toInt());

  m_childSegments.clear();
  QJsonArray segArray;
  QStringList segList;
  segList << "seasons" << "episodes" << "scenes" << "shots" << "subtitles";
    
  for (int i = 0; i < segList.size(); i++)
    if (json.contains(segList[i])) {
      segArray = json[segList[i]].toArray();
      for (int j = 0; j < segArray.size(); j++) {
	QJsonObject segObject = segArray[j].toObject();
	Segment *segment = nullptr;

	if (segList[i] == "seasons")
	  segment = new Season(this);
	else if (segList[i] == "episodes")
	  segment = new Episode(this);
	else if (segList[i] == "scenes")
	  segment = new Scene(this);
	else if (segList[i] == "shots")
	  segment = new Shot(this);
	
	segment->read(segObject);
	appendChild(segment);
      }
      break;
    }

  return;
}

void Segment::write(QJsonObject &json) const
{
  json["pos"] = m_position;
  json["source"] = m_source;

  if (childCount() > 0) {
    QString segType = "segments";
    if (dynamic_cast<Season *>(m_childSegments[0]))
      segType = "seasons";
    else if (dynamic_cast<Episode *>(m_childSegments[0]))
      segType = "episodes";
    else if (dynamic_cast<Scene *>(m_childSegments[0]))
      segType = "scenes";
    else if (dynamic_cast<Shot *>(m_childSegments[0]))
      segType = "shots";

    QJsonArray segArray;
    foreach (Segment *segment, m_childSegments) {
      QJsonObject segObject;
      segment->write(segObject);
      segArray.append(segObject);
    }

    json[segType] = segArray;
  }
  
  return;
}

////////////////////////////////////////
// methods dedicated to tree handling //
////////////////////////////////////////

Segment * Segment::child(int row)
{
  return m_childSegments.value(row);
}

void Segment::appendChild(Segment *childSegment)
{
  m_childSegments.append(childSegment);
}

void Segment::insertChild(int row, Segment *segment)
{
  m_childSegments.insert(row, segment);
}

bool Segment::insertChildren(int position, QList<Segment *> segments)
{
  if (position < 0 || position > m_childSegments.size())
    return false;

  for (int i(segments.size() - 1); i >= 0 ; i--)
    m_childSegments.insert(position, segments[i]);

  return true;
}

void Segment::removeChild(int row)
{
  m_childSegments.removeAt(row);
}

bool Segment::removeChildren(int position, int count)
{
  if (position < 0 || position > m_childSegments.size())
    return false;

  for (int i(count - 1); i >= 0; i--)
    delete m_childSegments.takeAt(position + i);

  return true;
}

int Segment::childCount() const
{
  return m_childSegments.count();
}

int Segment::row() const
{
  if (m_parentSegment)
    return m_parentSegment->m_childSegments.indexOf(const_cast<Segment *>(this));

  return 0;
}

Segment * Segment::parent()
{
  return m_parentSegment;
}

void Segment::splitChildren(QList<Segment *> &subList1, QList<Segment *> &subList2, int pos, Segment *newParent)
{
  int size = childCount();

  for (int i = 0; i < size; i++) {
    if (i < pos)
      subList1.append(m_childSegments[i]);
    else {
      m_childSegments[i]->setParent(newParent);
      subList2.append(m_childSegments[i]);
    }
  }
}

///////////////
// modifiers //
///////////////

void Segment::setPosition(qint64 position)
{
  m_position = position;
}

void Segment::setChildren(const QList<Segment *> &children)
{
  m_childSegments = children;
}

void Segment::setParent(Segment *parent)
{
  m_parentSegment = parent;
}

void Segment::setSource(Source source)
{
  m_source = source;
}

void Segment::clearChildren()
{
  m_childSegments.clear();
}

///////////////
// accessors //
///////////////

QString Segment::getFormattedPosition() const
{
  QString m_timeFormat;
  int ms = m_position % 1000;
  int secPos = m_position / 1000;

  QTime totalTime((secPos / 3600) % 60, (secPos / 60) % 60, secPos % 60, ms);

  if (secPos > 3600)
    m_timeFormat = "hh:mm:ss.zzz";
  else
    m_timeFormat = "mm:ss.zzz";

  return totalTime.toString(m_timeFormat);
}

qint64 Segment::getPosition() const
{
  return m_position;
}

QList<Segment *> Segment::getChildren() const
{
  return m_childSegments;
}

void Segment::setLabel(const QString &label, Segment::Source source)
{
  Q_UNUSED(label);
  Q_UNUSED(source);
}

void Segment::addInterLoc(const QString &speaker, SpkInteractDialog::InteractType type)
{
  Q_UNUSED(speaker);
  Q_UNUSED(type);
}

void Segment::resetInterLocs(SpkInteractDialog::InteractType type)
{
  Q_UNUSED(type);
}

void Segment::setEnd(qint64 end)
{
  Q_UNUSED(end);
}

int Segment::getNumber() const
{
  return -1;
}

qint64 Segment::getEnd() const
{
  return -1;
}

QString Segment::getLabel(Segment::Source source) const
{
  Q_UNUSED(source);

  return QString();
}

QStringList Segment::getInterLocs(SpkInteractDialog::InteractType type) const
{
  Q_UNUSED(type);

  return QStringList();
}

int Segment::getCamera(Segment::Source source) const
{
  Q_UNUSED(source);
  
  return -1;
}

int Segment::getHeight() const
{
  if (childCount() == 0)
    return 0;
  
  int maxHeight(-1);

  for (int i(0); i < childCount(); i++) {
    
    int currHeight = m_childSegments[i]->getHeight();
    
    if (currHeight > maxHeight)
      maxHeight = currHeight;
  }

  return 1 + maxHeight;
}

Segment::Source Segment::getSource() const
{
  return m_source;
}

int Segment::childIndexFromPosition(qint64 position)
{
  int size = childCount();
  bool found = false;
  int iSup(size-1);
  int iInf(0);
  int iMed(-1);
  qreal currPosition;
  
  while (!found && iSup >= iInf) {
    iMed = (iSup + iInf) / 2;
    currPosition = m_childSegments[iMed]->getPosition();
    if (position == currPosition)
      found = true;
    else if (position < currPosition)
      iSup = iMed - 1;
    else if (position > currPosition)
      iInf = iMed + 1;
  }

  // adjust index if element not found
  while (iMed < size - 1 && m_childSegments[iMed]->getPosition() <= position)
    iMed++;

  while (iMed > 0 && m_childSegments[iMed]->getPosition() > position)
    iMed--;
  
  return iMed;
}
