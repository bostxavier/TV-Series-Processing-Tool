#include <QSize>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QMediaPlayer>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QDir>
#include <QProcess>
#include <QtMath>

#include <opencv2/imgproc/imgproc.hpp>

#include "ProjectModel.h"
#include "Season.h"
#include "Scene.h"
#include "ResultsDialog.h"

using namespace std;
using namespace arma;
using namespace cv;

/////////////////
// constructor //
/////////////////

ProjectModel::ProjectModel(QObject *parent)
  : QAbstractItemModel(parent),
    m_name(QString()),
    m_baseName(QString())
{
  m_series = new Series;
  m_movieAnalyzer = new MovieAnalyzer;

  connect(m_movieAnalyzer, SIGNAL(insertShot(qint64, qint64)), this, SLOT(insertShotAuto(qint64, qint64)));
  connect(m_movieAnalyzer, SIGNAL(setCurrShot(qint64)), this, SLOT(setCurrShot(qint64)));
  connect(m_movieAnalyzer, SIGNAL(appendFaces(qint64, const QList<QRect> &)), this, SLOT(appendFaces(qint64, const QList<QRect> &)));
  connect(m_movieAnalyzer, SIGNAL(externFaceDetection(const QString &)), this, SLOT(externFaceDetection(const QString &)));
  connect(m_movieAnalyzer, SIGNAL(setSpkDiarData(const arma::mat, const arma::mat, const arma::mat)), this, SLOT(setSpkDiarData(const arma::mat, const arma::mat, const arma::mat)));
  connect(m_movieAnalyzer, SIGNAL(insertMusicRate(QPair<qint64, qreal>)), this, SLOT(insertMusicRate(QPair<qint64, qreal>)));
  connect(m_movieAnalyzer, SIGNAL(getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)), this, SLOT(getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)));
  connect(m_movieAnalyzer, SIGNAL(playLsus(QList<QPair<Episode *, QPair<qint64, qint64> > >)), this, SLOT(playSegments(QList<QPair<Episode *, QPair<qint64, qint64> > >)));
}

////////////////
// destructor //
////////////////

ProjectModel::~ProjectModel()
{
  delete m_series;
}

/////////////////////////
// save / open methods //
/////////////////////////

bool ProjectModel::save(const QString &fName)
{
  QFile saveFile(fName + ".json");
  // QFile saveFile(fName + ".dat");

  if (!saveFile.open(QIODevice::WriteOnly)) {
    qWarning("Couldn't save file.");
    return false;
  }

  QJsonObject projObject;
  projObject["name"] = m_name;

  QJsonObject seriesObject;
  m_series->write(seriesObject);
  projObject["series"] = seriesObject;

  QJsonDocument saveDoc(projObject);
  saveFile.write(saveDoc.toJson());
  // saveFile.write(saveDoc.toBinaryData());
  
  return true;
}

bool ProjectModel::load(const QString &fName)
{
  QFile loadFile(fName);

  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning("Couldn't open file.");
    return false;
  }

  QByteArray loadData = loadFile.readAll();

  QJsonDocument loadDoc(QJsonDocument::fromJson(loadData));
  // QJsonDocument loadDoc(QJsonDocument::fromBinaryData(loadData));
  QJsonObject projObject = loadDoc.object();
  
  m_name = projObject["name"].toString();
  QJsonObject seriesObject = projObject["series"].toObject();
  
  m_series->read(seriesObject);

  return true;
}

//////////////////////////////////////////
// reimplementation of abstract methods //
//  inherited from QAbstractItemModel   //
//////////////////////////////////////////

QModelIndex ProjectModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  Segment *parentSegment;

  if (!parent.isValid())
    parentSegment = m_series;
  else
    parentSegment = static_cast<Segment *>(parent.internalPointer());

  Segment *childSegment = parentSegment->child(row);

  if (childSegment)
    return createIndex(row, column, childSegment);
  else
    return QModelIndex();
}

QModelIndex ProjectModel::parent(const QModelIndex &child) const
{
  if (!child.isValid())
    return QModelIndex();
  
  Segment *childSegment = static_cast<Segment *>(child.internalPointer());
  Segment *parentSegment = childSegment->parent();

  if (parentSegment == m_series)
    return QModelIndex();

  return createIndex(parentSegment->row(), 0, parentSegment);
}

int ProjectModel::rowCount(const QModelIndex &parent) const
{
  Segment *parentSegment;

  if (!parent.isValid())
    parentSegment = m_series;
  else
    parentSegment = static_cast<Segment *>(parent.internalPointer());

  return parentSegment->childCount();
}

int ProjectModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent)

  return 2;
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  Segment *segment = static_cast<Segment *>(index.internalPointer());

  switch (role) {
  case Qt::DisplayRole:
    if (index.column() == 0)
      return segment->display();
    else
      return segment->getFormattedPosition();
    break;
  case Qt::ForegroundRole:
    if (index.column() == 1) {
      QBrush grayForeground(Qt::gray);
      return grayForeground;
    }
    break;
  case Qt::FontRole:
    QFont font;
    if (segment->getSource() == Segment::Automatic)
      font.setItalic(true);
    else if (segment->getSource() == Segment::Both)
      font.setBold(true);
    return font;
    break;
  }
   
  return QVariant();
}

QVariant ProjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
    return m_series->display();

  return QVariant();
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0;
  
  return QAbstractItemModel::flags(index);
}

bool ProjectModel::insertRows(int position, int rows, const QModelIndex &parent)
{
  Segment *parentSegment;

  if (!parent.isValid())
    parentSegment = m_series;
  else
    parentSegment = static_cast<Segment *>(parent.internalPointer());
    
  bool success;

  beginInsertRows(parent, position, position + rows - 1);
  success = parentSegment->insertChildren(position, m_segmentsToInsert);
  endInsertRows();

  return success;
}

bool ProjectModel::removeRows(int position, int rows, const QModelIndex &parent)
{
  Segment *parentSegment;

  if (!parent.isValid())
    parentSegment = m_series;
  else
    parentSegment = static_cast<Segment *>(parent.internalPointer());
    
  bool success;

  beginRemoveRows(parent, position, position + rows - 1);
  success = parentSegment->removeChildren(position, rows);
  endRemoveRows();

  return success;
}

QModelIndex ProjectModel::indexFromSegment(Segment *segment) const
{
  if (segment == m_series)
    return QModelIndex();

  return index(segment->row(), 0, indexFromSegment(segment->parent()));
}

int ProjectModel::getDepth() const
{
  return m_series->getHeight();
}

///////////////
// modifiers //
///////////////

void ProjectModel::initEpisodes()
{
  QList<Episode *> episodes;
  initEpisodes_aux(m_series, episodes);
  emit setEpisodes(episodes);
}
 
void ProjectModel::initEpisodes_aux(Segment *segment, QList<Episode *> &episodes)
{
  Episode *episode;

  if ((episode = dynamic_cast<Episode *>(segment))) {
    episodes.push_back(episode);
    emit addMedia(episode->getFName());
  }
  else
    for (int i(0); i < segment->childCount(); i++)
      initEpisodes_aux(segment->child(i), episodes);
}

void ProjectModel::setModel(const QString &name, const QString &seriesName, int seasNbr, int epNbr, const QString &epName, const QString &epFName)
{
  m_name = name;
  m_series->setName(seriesName);

  // season to insert
  Season *season = new Season(seasNbr, m_series);

  // episode to insert
  Episode *episode = new Episode(epNbr, epFName, season, epName);
  episode->setResolution(m_movieAnalyzer->getResolution(epFName));
  episode->setFps(m_movieAnalyzer->getFps(epFName));
  season->appendChild(episode);

  // first scene
  Scene *scene = new Scene(0, episode);
  episode->appendChild(scene);
  
  // update view
  QList<Segment *> segmentsToInsert;
  segmentsToInsert.push_back(season);
  setSegmentsToInsert(segmentsToInsert);
  insertRows(0, segmentsToInsert.size());
  emit setDepthView(getDepth());

  // extracting shots
  setEpisode(episode);
  emit updateEpisode(episode);
  m_movieAnalyzer->extractShots(epFName);
}

bool ProjectModel::addNewEpisode(int seasNbr, int epNbr, const QString &epName, const QString &epFName)
{
  // season of new episode
  Season *season = new Season(seasNbr, 0);

  // new episode
  Episode *episode = new Episode(epNbr, epFName, season, epName);
  episode->setResolution(m_movieAnalyzer->getResolution(epFName));
  episode->setFps(m_movieAnalyzer->getFps(epFName));
  season->appendChild(episode);
  
  // first scene
  Scene *scene = new Scene(0, episode);
  episode->appendChild(scene);

  // inserting new episode
  insertEpisode(episode);
  
  // update view
  emit setDepthView(getDepth());

  // extracting shots
  setEpisode(episode);
  emit updateEpisode(episode);

  m_movieAnalyzer->extractShots(epFName);

  return true;
}

bool ProjectModel::addModel(const QString &projectName, const QString &secProjectFName)
{
  // reinitializing project name
  m_name = projectName;
  QFile loadFile(secProjectFName);

  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning("Couldn't open file.");
    return false;
  }

  QByteArray loadData = loadFile.readAll();

  QJsonDocument loadDoc(QJsonDocument::fromJson(loadData));
  // QJsonDocument loadDoc(QJsonDocument::fromBinaryData(saveData));
  QJsonObject projObject = loadDoc.object();
  QJsonObject seriesObject = projObject["series"].toObject();

  Series *series = new Series;
  series->read(seriesObject);
  
  // series are different
  if (series->getName().toLower() != m_series->getName().toLower())
    return false;

  insertEpisodes(series);

  return true;
}

void ProjectModel::insertEpisodes(Segment *segment)
{
  Episode *episode;

  if ((episode = dynamic_cast<Episode *>(segment)))
    insertEpisode(episode);
  else
    for (int i(0); i < segment->childCount(); i++)
      insertEpisodes(segment->child(i));
}

void ProjectModel::insertEpisode(Segment *episode)
{
  Segment *newEpSeason = episode->parent();
  QList<Segment *> seasons = m_series->getChildren();
  int seasNbr = newEpSeason->getNumber();
  int epNbr = episode->getNumber();

  // creating season of new episode
  int i(0);
  int index(0);
  while (i < seasons.size() && seasNbr > seasons[i]->getNumber()) {
    index += seasons[i]->childCount();
    i++;
  }

  // season has not been inserted yet
  if (i == seasons.size() || seasNbr < seasons[i]->getNumber()) {

    // season to insert
    newEpSeason->setParent(m_series);

    QList<Segment *> segmentsToInsert;
    segmentsToInsert.push_back(newEpSeason);
    setSegmentsToInsert(segmentsToInsert);

    // insert season
    insertRows(i, segmentsToInsert.size());

    // insert media
    emit insertEpisode(index, (dynamic_cast<Episode *>(episode)));
    emit insertMedia(index, (dynamic_cast<Episode *>(episode))->getFName());
  }
  
  // season already exists
  else {
    newEpSeason = seasons[i];

    // inserting episode
    QList<Segment *> episodes = newEpSeason->getChildren();

    i = 0;
    while (i < episodes.size() && epNbr > episodes[i]->getNumber()) {
      index++;
      i++;
    }

    // episode has not been inserted yet: insert it
    if (i == episodes.size() || epNbr < episodes[i]->getNumber()) {
    
      // episode to insert
      episode->setParent(newEpSeason);
      QList<Segment *> segmentsToInsert;
      segmentsToInsert.push_back(episode);
      setSegmentsToInsert(segmentsToInsert);

      // index of season within model
      QModelIndex parent = indexFromSegment(newEpSeason);
    
      // insert episode
      insertRows(i, segmentsToInsert.size(), parent);

      // insert media
      emit insertEpisode(index, (dynamic_cast<Episode *>(episode)));
      emit insertMedia(index, (dynamic_cast<Episode *>(episode))->getFName());
    }
  }
}

void ProjectModel::setSegmentsToInsert(QList<Segment *> segmentsToInsert)
{
  m_segmentsToInsert = segmentsToInsert;
}

bool ProjectModel::insertSubtitles(const QString &subFName)
{
  QList<SpeechSegment *> speechSegments;
  QJsonObject subObject;
  qint64 start;
  qint64 end;
  qint64 totDuration;
  QString text;
  QStringList sources;
  QList<int> absLength;
  QList<qreal> relLength;
  int totLength;
  qreal startShift(-0.38);
  qreal endShift(-0.5);

  // regular expression to detect subtitles corresponding to noise
  QRegularExpression noiseSource("^\\s*\\(.*\\)\\s*$");

  // regular expression to detect subtitles with multiple speakers
  QRegularExpression multSources("[_-].+<br />[_-].+");

  QFile loadFile(subFName);

  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning("Couldn't open subtitles file.");
    return false;
  }

  QByteArray saveData = loadFile.readAll();

  QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
  QJsonArray subArray = loadDoc.array();
  
  for (int i(0); i < subArray.size(); i++) {

    sources.clear();
    absLength.clear();
    relLength.clear();
    totLength = 0;

    subObject = subArray[i].toObject();
    start = (subObject["start"].toDouble() + startShift) * 1000;
    if (start < 0)
      start = 0;
    start = qRound(start / 10.0) * 10;
    end = (subObject["end"].toDouble() + endShift) * 1000;
    end = qRound(end / 10.0) * 10;
    text = subObject["text"].toString();
 
    QRegularExpressionMatch multSourcesMatch = multSources.match(text);

    // case of multiple source in current subtitle
    if (multSourcesMatch.hasMatch()) {

      // split subtitle contents
      sources = text.split("<br />");
      
      // removing hyphenation indicating speaker turn
      for (int k(0); k < sources.size(); k++) {
	sources[k].replace(QRegularExpression("\\s*-\\s*"), "");
	sources[k].replace(QRegularExpression("\\s*_\\s*"), "");
      }

      // estimate absolute length of each contents
      for (int k(0); k < sources.size(); k++) {
	absLength.push_back((sources[k]).count(" ") + 1);
	totLength += absLength[k];
      }
      
      // estimate relative length of each contents
      for (int k(0); k < sources.size(); k++)
	relLength.push_back(absLength[k] / static_cast<qreal>(totLength));

      // inserting multiple subtitles
      totDuration = end - start;
      for (int k(0); k < sources.size(); k++) {
	qint64 utterDuration = static_cast<qint64>(totDuration * relLength[k]);
	qint64 utterEnd = start + utterDuration;
	QRegularExpressionMatch noiseMatch = noiseSource.match(sources[k]);
	QVector<QString> speakers = {"", ""};
	QVector<QStringList> interLocs = {QStringList(), QStringList(), QStringList()};
	if (!noiseMatch.hasMatch())
	  speakers[0] = "S";

	speechSegments.push_back(new SpeechSegment(start,
						   utterEnd,
						   sources[k],
						   speakers,
						   interLocs));
	start = utterEnd;
      }
    }

    // case of a single source in current subtitle
    else {
      QRegularExpressionMatch noiseMatch = noiseSource.match(text);
      QVector<QString> speakers = {"", ""};
      QVector<QStringList> interLocs = {QStringList(), QStringList(), QStringList()};
      if (!noiseMatch.hasMatch())
	speakers[0] = "S";

      speechSegments.push_back(new SpeechSegment(start,
						 end,
						 text,
						 speakers,
						 interLocs));
    }
  }

  m_episode->setSpeechSegments(speechSegments);
  emit resetSegmentView();

  return true;
}

void ProjectModel::getDiarData()
{
  QList<SpeechSegment *> speechSegments;
  retrieveSpeechSegments(m_episode, speechSegments);

  m_movieAnalyzer->getSpkDiarData(speechSegments);
}

bool ProjectModel::localSpkDiar(SpkDiarizationDialog::Method method, UtteranceTree::DistType dist, bool norm, UtteranceTree::AgrCrit agr, UtteranceTree::PartMeth partMeth, bool weight, bool sigma)
{
  QList<SpeechSegment *> speechSegments;
  retrieveSpeechSegments(m_episode, speechSegments);
  speechSegments = m_movieAnalyzer->denoiseSpeechSegments(speechSegments);
  speechSegments = m_movieAnalyzer->filterSpeechSegments(speechSegments);

  switch (method) {
  case SpkDiarizationDialog::HC:
    m_movieAnalyzer->localSpkDiarHC(dist, norm, agr, partMeth, weight, sigma, speechSegments, m_lsuSpeechSegments);
    break;
  case SpkDiarizationDialog::Ref:
  default:
    break;
  }

  return true;
}

bool ProjectModel::globalSpkDiar()
{
  m_movieAnalyzer->globalSpkDiar(m_baseName, m_subBound, m_subRefLbl);

  return true;
}

void ProjectModel::spkInteract(SpkInteractDialog::InteractType type, SpkInteractDialog::RefUnit unit, int nbDiscards, int interThresh)
{
  // episodes

  //////////////////
  // breaking bad //
  //////////////////

  QVector<QPair<int, int> > episodes_bb = {QPair<int, int>(1, 1),
					   QPair<int, int>(1, 2),
					   QPair<int, int>(1, 3),
					   QPair<int, int>(1, 4),
					   QPair<int, int>(1, 5),
					   QPair<int, int>(1, 6),
					   QPair<int, int>(1, 7),
					   QPair<int, int>(2, 1),
					   QPair<int, int>(2, 2),
					   QPair<int, int>(2, 3),
					   QPair<int, int>(2, 4),
					   QPair<int, int>(2, 5),
					   QPair<int, int>(2, 6),
					   QPair<int, int>(2, 7),
					   QPair<int, int>(2, 8),
					   QPair<int, int>(2, 9),
					   QPair<int, int>(2, 10),
					   QPair<int, int>(2, 11),
					   QPair<int, int>(2, 12),
					   QPair<int, int>(2, 13),
					   QPair<int, int>(3, 1),
					   QPair<int, int>(3, 2),
					   QPair<int, int>(3, 3),
					   QPair<int, int>(3, 4),
					   QPair<int, int>(3, 5),
					   QPair<int, int>(3, 6),
					   QPair<int, int>(3, 7),
					   QPair<int, int>(3, 8),
					   QPair<int, int>(3, 9),
					   QPair<int, int>(3, 10),
					   QPair<int, int>(3, 11),
					   QPair<int, int>(3, 12),
					   QPair<int, int>(3, 13),
					   QPair<int, int>(4, 1),
					   QPair<int, int>(4, 2),
					   QPair<int, int>(4, 3),
					   QPair<int, int>(4, 4),
					   QPair<int, int>(4, 5),
					   QPair<int, int>(4, 6),
					   QPair<int, int>(4, 7),
					   QPair<int, int>(4, 8),
					   QPair<int, int>(4, 9),
					   QPair<int, int>(4, 10),
					   QPair<int, int>(4, 11),
					   QPair<int, int>(4, 12),
					   QPair<int, int>(4, 13),
					   QPair<int, int>(5, 1),
					   QPair<int, int>(5, 2),
					   QPair<int, int>(5, 3),
					   QPair<int, int>(5, 4),
					   QPair<int, int>(5, 5),
					   QPair<int, int>(5, 6),
					   QPair<int, int>(5, 7),
					   QPair<int, int>(5, 8),
					   QPair<int, int>(5, 9),
					   QPair<int, int>(5, 10),
					   QPair<int, int>(5, 11),
					   QPair<int, int>(5, 12),
					   QPair<int, int>(5, 13),
					   QPair<int, int>(5, 14),
					   QPair<int, int>(5, 15),
					   QPair<int, int>(5, 16)};

  /////////////////////
  // game of thrones //
  /////////////////////

  QVector<QPair<int, int> > episodes_got = {QPair<int, int>(1, 1),
					    QPair<int, int>(1, 2),
					    QPair<int, int>(1, 3),
					    QPair<int, int>(1, 4),
					    QPair<int, int>(1, 5),
					    QPair<int, int>(1, 6),
					    QPair<int, int>(1, 7),
					    QPair<int, int>(1, 8),
					    QPair<int, int>(1, 9),
					    QPair<int, int>(1, 10),
					    QPair<int, int>(2, 1),
					    QPair<int, int>(2, 2),
					    QPair<int, int>(2, 3),
					    QPair<int, int>(2, 4),
					    QPair<int, int>(2, 5),
					    QPair<int, int>(2, 6),
					    QPair<int, int>(2, 7),
					    QPair<int, int>(2, 8),
					    QPair<int, int>(2, 9),
					    QPair<int, int>(2, 10),
					    QPair<int, int>(3, 1),
					    QPair<int, int>(3, 2),
					    QPair<int, int>(3, 3),
					    QPair<int, int>(3, 4),
					    QPair<int, int>(3, 5),
					    QPair<int, int>(3, 6),
					    QPair<int, int>(3, 7),
					    QPair<int, int>(3, 8),
					    QPair<int, int>(3, 9),
					    QPair<int, int>(3, 10),
					    QPair<int, int>(4, 1),
					    QPair<int, int>(4, 2),
					    QPair<int, int>(4, 3),
					    QPair<int, int>(4, 4),
					    QPair<int, int>(4, 5),
					    QPair<int, int>(4, 6),
					    QPair<int, int>(4, 7),
					    QPair<int, int>(4, 8),
					    QPair<int, int>(4, 9),
					    QPair<int, int>(4, 10),
					    QPair<int, int>(5, 1),
					    QPair<int, int>(5, 2),
					    QPair<int, int>(5, 3),
					    QPair<int, int>(5, 4),
					    QPair<int, int>(5, 5),
					    QPair<int, int>(5, 6),
					    QPair<int, int>(5, 7),
					    QPair<int, int>(5, 8),
					    QPair<int, int>(5, 9),
					    QPair<int, int>(5, 10),
					    QPair<int, int>(6, 1),
					    QPair<int, int>(6, 2),
					    QPair<int, int>(6, 3),
					    QPair<int, int>(6, 4),
					    QPair<int, int>(6, 5),
					    QPair<int, int>(6, 6),
					    QPair<int, int>(6, 7),
					    QPair<int, int>(6, 8),
					    QPair<int, int>(6, 9),
					    QPair<int, int>(6, 10),
					    QPair<int, int>(7, 1),
					    QPair<int, int>(7, 2),
					    QPair<int, int>(7, 3),
					    QPair<int, int>(7, 4),
					    QPair<int, int>(7, 5),
					    QPair<int, int>(7, 6),
					    QPair<int, int>(7, 7)};

  ////////////////////
  // house of cards //
  ////////////////////

  QVector<QPair<int, int> > episodes_hoc = {QPair<int, int>(1, 1),
					    QPair<int, int>(1, 2),
					    QPair<int, int>(1, 3),
					    QPair<int, int>(1, 4),
					    QPair<int, int>(1, 5),
					    QPair<int, int>(1, 6),
					    QPair<int, int>(1, 7),
					    QPair<int, int>(1, 8),
					    QPair<int, int>(1, 9),
					    QPair<int, int>(1, 10),
					    QPair<int, int>(1, 11),
					    QPair<int, int>(1, 12),
					    QPair<int, int>(1, 13),
					    QPair<int, int>(2, 1),
					    QPair<int, int>(2, 2),
					    QPair<int, int>(2, 3),
					    QPair<int, int>(2, 4),
					    QPair<int, int>(2, 5),
					    QPair<int, int>(2, 6),
					    QPair<int, int>(2, 7),
					    QPair<int, int>(2, 8),
					    QPair<int, int>(2, 9),
					    QPair<int, int>(2, 10),
					    QPair<int, int>(2, 11),
					    QPair<int, int>(2, 12),
					    QPair<int, int>(2, 13)};

  // select n most representative episode(s) in terms of # speakers / scene
  // QVector<QPair<int, int> > repr = getEpisodeRepr(episodes_hoc);

  // QVector<QPair<int, int> > repr = {episodes_bb[3], episodes_bb[5], episodes_bb[9], episodes_bb[10]};
  // QVector<QPair<int, int> > repr = {episodes_got[2], episodes_got[6], episodes_got[7]};
  // QVector<QPair<int, int> > repr = {episodes_hoc[0], episodes_hoc[6], episodes_hoc[10]};
  
  // retrieve speech segments by unit
  QList<QList<SpeechSegment *> > unitSpeechSegments;
  QList<QList<SpeechSegment *> > refUnitSpeechSegments;
  QList<Scene *> scenes;
  
  if (unit == SpkInteractDialog::Scene)
    retrieveSpeakersNet_aux(m_series, unitSpeechSegments, scenes);
  else if (unit == SpkInteractDialog::LSU)
    retrieveSpeakersNet_aux(m_series, unitSpeechSegments, scenes, episodes_hoc, true);

  retrieveSpeakersNet_aux(m_series, refUnitSpeechSegments, scenes);

  // displayStats(unitSpeechSegments);

  // performance matrices
  arma::mat S(0, 5);
  arma::mat U(0, 5);

  // step-by-step evaluation of each rule
  for (int i(3); i < 4; i++) {

    // initializing vector of rules
    QVector<bool> rules(4, false);
    for (int j(0); j <= i; j++)
      rules[j] = true;

    // clear speech segments
    for (int i(0); i < unitSpeechSegments.size(); i++)
      for (int j(0); j < unitSpeechSegments[i].size(); j++)
	unitSpeechSegments[i][j]->resetInterLocs(type);

    // estimate speaker interactions
    if (type == SpkInteractDialog::CoOccurr)
      m_movieAnalyzer->setCoOccurrInteract(unitSpeechSegments, nbDiscards);
    else
      m_movieAnalyzer->setSequentialInteract(unitSpeechSegments, nbDiscards, interThresh, rules);
    
    // retrieve reference and hypothesized speaker-oriented networks
    QList<QMap<QString, QMap<QString, qreal> > > conversNets;
    conversNets.push_back(getSpkOrientedNet(refUnitSpeechSegments, SpkInteractDialog::Ref));
    conversNets.push_back(getSpkOrientedNet(unitSpeechSegments, type));

    // retrieve reference and hypothesized utterance-oriented networks
    conversNets.push_back(getUttOrientedNet(refUnitSpeechSegments, SpkInteractDialog::Ref));
    conversNets.push_back(getUttOrientedNet(unitSpeechSegments, type));

    // compute and possibly display evaluation results
    QPair<QVector<qreal>, QVector<qreal> > results = evaluateSpkInteract(unitSpeechSegments, true, type, conversNets);

    // S.insert_rows(i, arma::mat(results.first.toStdVector()).t());
    // U.insert_rows(i, arma::mat(results.second.toStdVector()).t());
  }

  // save results
  // cout << S << endl;
  // cout << U << endl;
  // S.save("/home/xavier/Dropbox/R/dist/hoc_spk_rules.dat", raw_ascii);
  // U.save("/home/xavier/Dropbox/R/dist/hoc_utt_rules.dat", raw_ascii);
}

void ProjectModel::displayStats(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  int nUtter(0);
  int nSpkOcc(0);
  qreal totSpeech(0.0);
  int nSpkScenes(0);
  qreal meanSpkScene(0.0);
  qreal devSpkScene(0.0);

  for (int i(0); i < sceneSpeechSegments.size(); i++) {

    QStringList speakers;

    // looping over speech segments
    for (int j(0); j < sceneSpeechSegments[i].size(); j++) {

      QString currSpeaker = sceneSpeechSegments[i][j]->getLabel(Segment::Manual);
      qreal duration = (sceneSpeechSegments[i][j]->getEnd() - sceneSpeechSegments[i][j]->getPosition()) / 1000.0;

      totSpeech += duration;

      // new speaker within current scene
      if (!speakers.contains(currSpeaker)) {
	nSpkOcc++;
	speakers.push_back(currSpeaker);
      }
    }

    // update number of speakers
    nUtter += sceneSpeechSegments[i].size();
    meanSpkScene += speakers.size();
    if (sceneSpeechSegments[i].size() > 0)
      nSpkScenes++;
  }

  // # speakers / scene: standard deviation
  meanSpkScene /= sceneSpeechSegments.size();

  for (int i(0); i < sceneSpeechSegments.size(); i++) {

    QStringList speakers;

    // looping over speech segments
    for (int j(0); j < sceneSpeechSegments[i].size(); j++) {

      QString currSpeaker = sceneSpeechSegments[i][j]->getLabel(Segment::Manual);
      // new speaker within current scene
      if (!speakers.contains(currSpeaker))
	speakers.push_back(currSpeaker);
    }

    // update number of speakers
    devSpkScene += qPow(speakers.size() - meanSpkScene, 2);
  }

  devSpkScene /= sceneSpeechSegments.size();
  devSpkScene = qSqrt(devSpkScene);

  qDebug() << "# scenes:" << sceneSpeechSegments.size();
  qDebug() << "# speech segments:" << nUtter;
  qDebug() << "# speaker occurrences:" << nSpkOcc;
  qDebug() << "% spoken scenes:" << nSpkScenes  * 100.0 / sceneSpeechSegments.size();
  qDebug() << "speech dur:"<< totSpeech << "s.";
  qDebug() << "# speakers/scene (avg.):" << meanSpkScene;
  qDebug() << "# speakers/scene (dev.):" << devSpkScene;
}

arma::vec ProjectModel::computeSpkSceneDist(QList<QList<SpeechSegment *> > sceneSpeechSegments, int nBins)
{
  // histogram to return
  arma::vec H;

  // set number of bins
  if (nBins == -1) {
    
    for (int i(0); i < sceneSpeechSegments.size(); i++) {

      QStringList speakers;
      int nSpeakers(0);

      // looping over speech segments
      for (int j(0); j < sceneSpeechSegments[i].size(); j++) {

	QString currSpeaker = sceneSpeechSegments[i][j]->getLabel(Segment::Manual);

	// new speaker within current scene
	if (!speakers.contains(currSpeaker))
	  speakers.push_back(currSpeaker);
      }

      // Possibly updating number of bins
      nSpeakers = speakers.size();
      if (nSpeakers > nBins)
	nBins = nSpeakers;
    }

    nBins++;
  }

  // fill histograms
  H.zeros(nBins);
  for (int i(0); i < sceneSpeechSegments.size(); i++) {

    QStringList speakers;

    // looping over speech segments
    for (int j(0); j < sceneSpeechSegments[i].size(); j++) {

      QString currSpeaker = sceneSpeechSegments[i][j]->getLabel(Segment::Manual);

      // new speaker within current scene
      if (!speakers.contains(currSpeaker))
	speakers.push_back(currSpeaker);
    }

    // updating histogram
    int nSpeakers = speakers.size();

    H(nSpeakers) += sceneSpeechSegments[i].size();
    // H(nSpeakers)++;
  }

  // normalizing histogram
  H /= sum(H);

  return H;
}

QVector<QPair<int, int> > ProjectModel::getEpisodeRepr(const QVector<QPair<int, int> > &episodes)
{
  int n(episodes.size());
  QVector<QPair<int, int> > repr;

  // compute P(segment belongs to a scene with i speaker(s))
  // for all episodes
  QList<QList<SpeechSegment *> > sceneSpeechSegments;
  QList<Scene *> scenes;
  retrieveSpeakersNet_aux(m_series, sceneSpeechSegments, scenes, episodes);
  arma::vec HA = computeSpkSceneDist(sceneSpeechSegments);

  ////////
  // bb //
  ////////

  /*
  repr.push_back(QPair<int, int>(2, 3));
  repr.push_back(QPair<int, int>(2, 4));
  repr.push_back(QPair<int, int>(1, 4));
  repr.push_back(QPair<int, int>(1, 6));
  */

  /////////
  // got //
  /////////

  /*
  repr.push_back(QPair<int, int>(1, 3));
  repr.push_back(QPair<int, int>(1, 7));
  repr.push_back(QPair<int, int>(1, 8));
  */

  /////////
  // hoc //
  /////////

  repr.push_back(QPair<int, int>(1, 7));
  repr.push_back(QPair<int, int>(1, 11));
  // repr.push_back(QPair<int, int>(1, 1));

  // compute P(segment belongs to a scene with n speaker(s))
  // for each episode subset of cardinal k
  // and get most representative subset
  int k(1);
  QVector<QPair<int, int> > currSubset(3);
  currSubset[0] = repr[0];
  currSubset[1] = repr[1];
  QVector<QPair<int, int> > bestSubset;
  qreal minDist(HA.n_rows);
  
  // retrieve the most representative subset of episodes
  getBestSubset(episodes, currSubset, k, -1, HA, minDist, repr);

  // compute P(segment belongs to a scene with i speaker(s))
  // for the most representative subset of episodes
  sceneSpeechSegments.clear();
  retrieveSpeakersNet_aux(m_series, sceneSpeechSegments, scenes, repr);
  arma::vec HE = computeSpkSceneDist(sceneSpeechSegments, HA.n_rows);

  // distance to global distribution
  qreal d = as_scalar(norm(HA - HE, 1));
  
  arma::mat D = join_rows(HA, HE);
  qDebug() << repr << endl;
  cout << D << endl;
  qDebug() << d;
  
  // D.save("/home/xavier/Dropbox/R/dist/hoc_spk_scene_spk.dat", raw_ascii);

  return repr;
}

void ProjectModel::getBestSubset(const QVector<QPair<int, int> > &episodes, QVector<QPair<int, int> > &currSubset, int k, int i, arma::vec HA, qreal &minDist, QVector<QPair<int, int> > &bestSubset)
{
  // subset has reached k elements
  if (k == 0) {
    
    // compute P(segment belongs to a scene with i speaker(s))
    // for current subset of episodes
    QList<QList<SpeechSegment *> > sceneSpeechSegments;
    QList<Scene *> scenes;
    retrieveSpeakersNet_aux(m_series, sceneSpeechSegments, scenes, currSubset);
    arma::vec HE = computeSpkSceneDist(sceneSpeechSegments, HA.n_rows);

    // distance to global distribution
    qreal d = as_scalar(norm(HA - HE, 1));

    if (d <= minDist) {
      minDist = d;
      bestSubset = currSubset;
    }

    return;
  }

  for (int j(i+1); j < episodes.size(); j++) {
    currSubset[currSubset.size()-k] = episodes[j];
    getBestSubset(episodes, currSubset, k-1, j, HA, minDist, bestSubset);
  }
}

QPair<QVector<qreal>, QVector<qreal> > ProjectModel::evaluateSpkInteract(QList<QList<SpeechSegment *> > unitSpeechSegments, bool displayResults, SpkInteractDialog::InteractType type, const QList<QMap<QString, QMap<QString, qreal> > > &conversNets)
{
  qreal precision(0.0);
  qreal recall(0.0);
  qreal fScore(0.0);
  qreal accuracy(0.0);
  qreal coverage(0.0);
  qreal cos(-1.0);
  qreal l2(-1.0);
  qreal jaccard(-1.0);
  int nSeg(0);
  int nSpk(0);
  int nUsed(0);
  int nEval(0);

  // evaluation (1): speaker-oriented
  QVector<qreal> resSpkOriented;
  QVector<qreal> resUttOriented;

  // loop over units
  for (int i(0); i < unitSpeechSegments.size(); i++) {

    QStringList speakers;
    QMap<QString, QStringList> spkRefInterloc;
    QMap<QString, QStringList> spkHypInterloc;

    for (int j(0); j < unitSpeechSegments[i].size(); j++) {

      nSeg++;
	
      // current speaker
      QString currSpeaker = unitSpeechSegments[i][j]->getLabel(Segment::Manual);

      // update speakers list
      if (!speakers.contains(currSpeaker)) {
	speakers.push_back(currSpeaker);
	nUsed++;
      }
      
      // current interlocutors
      QStringList refInterLoc = unitSpeechSegments[i][j]->getInterLocs(SpkInteractDialog::Ref);
      QStringList hypInterLoc = unitSpeechSegments[i][j]->getInterLocs(type);

      // loop over reference interlocutors
      for (int k(0); k < refInterLoc.size(); k++) {
	if (!spkRefInterloc[currSpeaker].contains(refInterLoc[k])) {
	  spkRefInterloc[currSpeaker].push_back(refInterLoc[k]);
	}
      }
      
      // loop over hypothesized interlocutors
      for (int k(0); k < hypInterLoc.size(); k++) {
	if (!spkHypInterloc[currSpeaker].contains(hypInterLoc[k])) {
	  spkHypInterloc[currSpeaker].push_back(hypInterLoc[k]);
	}
      }
    }

    // loop over speakers
    qSort(speakers);
    for (int j(0); j < speakers.size(); j++) {

      // interlocutors have been hypothesized for current segment
      if (spkHypInterloc[speakers[j]].size() > 0) {

	// possibly add null element
	if (spkRefInterloc[speakers[j]].isEmpty())
	  spkRefInterloc[speakers[j]].push_back("NULL");
	
	int nInter = cardInter(spkHypInterloc[speakers[j]], spkRefInterloc[speakers[j]]);
	int nUnion = cardUnion(spkHypInterloc[speakers[j]], spkRefInterloc[speakers[j]]);

	precision += static_cast<qreal>(nInter) / spkHypInterloc[speakers[j]].size();
	recall += static_cast<qreal>(nInter) / spkRefInterloc[speakers[j]].size();
	accuracy += static_cast<qreal>(nInter) / nUnion;

	nEval++;
      }
    }

    nSpk += speakers.size();
  }
    
  if (nEval > 0) {
    precision /= nEval;
    recall /= nEval;
    accuracy /= nEval;
    fScore = 2 * precision * recall / (precision + recall);
    coverage = static_cast<qreal>(nUsed) / nSeg;
    // coverage = static_cast<qreal>(nEval) / nSpk;
    cos = m_movieAnalyzer->cosineSim(conversNets[0], conversNets[1]);
    l2 = m_movieAnalyzer->l2Dist(conversNets[0], conversNets[1]);
    jaccard = m_movieAnalyzer->jaccardIndex(conversNets[0], conversNets[1]);

    resSpkOriented.push_back(coverage);
    resSpkOriented.push_back(fScore);
    resSpkOriented.push_back(cos);
    resSpkOriented.push_back(l2);
    resSpkOriented.push_back(jaccard);

    if (displayResults) {
      ResultsDialog *dialog = new ResultsDialog(0, 0, precision, recall, fScore, accuracy, cos, l2, jaccard, coverage);
      dialog->exec();
    }
  }

  // evaluation (2): utterance-oriented
  precision = 0.0;
  recall = 0.0;
  fScore = 0.0;
  accuracy = 0.0;
  coverage = 0.0;
  nSeg = 0;
  nEval = 0;

  // loop over units
  for (int i(0); i < unitSpeechSegments.size(); i++)
    for (int j(0); j < unitSpeechSegments[i].size(); j++) {

      QString currSpk = unitSpeechSegments[i][j]->getLabel(Segment::Manual);
      QStringList refInterLoc = unitSpeechSegments[i][j]->getInterLocs(SpkInteractDialog::Ref);
      QStringList hypInterLoc = unitSpeechSegments[i][j]->getInterLocs(type);
      
      nSeg++;
  
      // interlocutors have been hypothesized for current segment
      if (hypInterLoc.size() > 0) {
	
	if (refInterLoc.isEmpty())
	  refInterLoc.push_back("NULL");

	int nInter = cardInter(hypInterLoc, refInterLoc);
	int nUnion = cardUnion(hypInterLoc, refInterLoc);

	precision += static_cast<qreal>(nInter) / hypInterLoc.size();
	recall += static_cast<qreal>(nInter) / refInterLoc.size();
	accuracy += static_cast<qreal>(nInter) / nUnion;

	nEval++;
      }
    }
  
  if (nEval > 0) {
    precision /= nEval;
    recall /= nEval;
    accuracy /= nEval;
    fScore = 2 * precision * recall / (precision + recall);
    coverage = static_cast<qreal>(nEval) / nSeg;
    cos = m_movieAnalyzer->cosineSim(conversNets[2], conversNets[3]);
    l2 = m_movieAnalyzer->l2Dist(conversNets[2], conversNets[3]);
    jaccard = m_movieAnalyzer->jaccardIndex(conversNets[2], conversNets[3]);

    resUttOriented.push_back(coverage);
    resUttOriented.push_back(fScore);
    resUttOriented.push_back(cos);
    resUttOriented.push_back(l2);
    resUttOriented.push_back(jaccard);

    // processErrorCases(unitSpeechSegments, hypInter, refInter);

    if (displayResults) {
      ResultsDialog *dialog = new ResultsDialog(0, 0, precision, recall, fScore, accuracy, cos, l2, jaccard, coverage);
      dialog->exec();
    }
  }

  return QPair<QVector<qreal>, QVector<qreal> >(resSpkOriented, resUttOriented);
}

QMap<QString, QMap<QString, qreal> > ProjectModel::getSpkOrientedNet(QList<QList<SpeechSegment *> > unitSpeechSegments, SpkInteractDialog::InteractType type)
{
  QMap<QString, QMap<QString, qreal> > spkOrientedNet;

  // loop over units
  for (int i(0); i < unitSpeechSegments.size(); i++) {

    QMap<QString, QStringList> spkInterloc;

    for (int j(0); j < unitSpeechSegments[i].size(); j++) {
	
      // current speaker
      QString currSpeaker = unitSpeechSegments[i][j]->getLabel(Segment::Manual);
      
      // current interlocutors
      QStringList interLoc = unitSpeechSegments[i][j]->getInterLocs(type);
     
      if (!interLoc.isEmpty()) {
	// loop over interlocutors
	for (int k(0); k < interLoc.size(); k++) {
	  if (!spkInterloc[currSpeaker].contains(interLoc[k])) {
	    spkInterloc[currSpeaker].push_back(interLoc[k]);
	    spkOrientedNet[currSpeaker][interLoc[k]]++;
	  }
	}
      }
    }
  }

  return spkOrientedNet;
}

QMap<QString, QMap<QString, qreal> > ProjectModel::getUttOrientedNet(QList<QList<SpeechSegment *> > unitSpeechSegments, SpkInteractDialog::InteractType type, bool nbWeight)
{
  QMap<QString, QMap<QString, qreal> > uttOrientedNet;

  // loop over units
  for (int i(0); i < unitSpeechSegments.size(); i++)
    for (int j(0); j < unitSpeechSegments[i].size(); j++) {

      QString currSpk = unitSpeechSegments[i][j]->getLabel(Segment::Manual);
      QStringList interLoc = unitSpeechSegments[i][j]->getInterLocs(type);
      
      if (!interLoc.isEmpty()) {

	// update interactions
	for (int k(0); k < interLoc.size(); k++)
	  if (nbWeight)
	    uttOrientedNet[currSpk][interLoc[k]] += 1.0 / interLoc.size();
	  else {
	    qreal duration = (unitSpeechSegments[i][j]->getEnd() - unitSpeechSegments[i][j]->getPosition()) / 1000.0;
	    uttOrientedNet[currSpk][interLoc[k]] += duration / interLoc.size();
	  }
      }
    }

  return uttOrientedNet;
}

void ProjectModel::processErrorCases(QList<QList<SpeechSegment *> > unitSpeechSegments, const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const
{
  int nErrors(0);
  int nNetErrors(0);

  // loop over units
  for (int i(0); i < unitSpeechSegments.size(); i++)
    for (int j(0); j < unitSpeechSegments[i].size(); j++) {

      QString currSpk = unitSpeechSegments[i][j]->getLabel(Segment::Manual);
      QVector<QStringList> interLocs = unitSpeechSegments[i][j]->getInterLocs();
      QStringList refInterLoc = interLocs[0];
      QStringList hypInterLoc = interLocs[1];
      
      // interlocutors have been hypothesized for current segment
      if (hypInterLoc.size() > 0) {
	
	if (refInterLoc.isEmpty())
	  refInterLoc.push_back("NULL");

	int nInter = cardInter(hypInterLoc, refInterLoc);

	qreal precision = static_cast<qreal>(nInter) / hypInterLoc.size();
	qreal recall = static_cast<qreal>(nInter) / refInterLoc.size();
	qreal fScore(0.0);
	if (precision > 0 || recall > 0)
	  fScore = 2 * precision * recall / (precision + recall);

	if (fScore == 0.0) {
	  
	  qDebug() << currSpk << hypInterLoc << refInter[currSpk][hypInterLoc.first()] << "/" << hypInter[currSpk][hypInterLoc.first()] << refInterLoc;
	  
	  if (refInter[currSpk][hypInterLoc.first()] == 0.0)
	    nNetErrors++;

	  nErrors++;
	}
      }
    }
 
 qDebug() << "\n% of misleading mistakes:" << QString::number(100.0 * nNetErrors / nErrors, 'g', 4);
}

int ProjectModel::cardInter(const QStringList &L1, const QStringList &L2) const
{
  int cardInter(0);

  if (L1.size() <= L2.size())
    for (int i(0); i < L1.size(); i++) {
      if (L2.contains(L1[i]))
	cardInter++;
    }
  else
    for (int i(0); i < L2.size(); i++) {
      if (L1.contains(L2[i]))
	cardInter++;
    }
	
  return cardInter;
}

int ProjectModel::cardUnion(const QStringList &L1, const QStringList &L2) const
{
  QStringList u;

  for (int i(0); i < L1.size(); i++)
    if (!u.contains(L1[i]))
      u.push_back(L1[i]);
  
  for (int i(0); i < L2.size(); i++)
    if (!u.contains(L2[i]))
      u.push_back(L2[i]);
  
  return u.size();
}

void ProjectModel::musicTracking(int frameRate, int mtWindowSize, int mtHopSize, int chromaStaticFrameSize, int chromaDynamicFrameSize)
{
  // clear possibly recorded music rate estimates
  QList<Shot *> shots;
  retrieveShots(m_episode, shots);

  for (int i(0); i < shots.size(); i++)
    shots[i]->clearMusicRates();

  m_movieAnalyzer->musicTracking(m_episode->getFName(), frameRate, mtWindowSize, mtHopSize, chromaStaticFrameSize, chromaDynamicFrameSize);
}

bool ProjectModel::coClustering(const QString &fName)
{
  QList<Shot *> shots;
  retrieveShots(m_episode, shots);

  QList<SpeechSegment *> speechSegments;
  retrieveSpeechSegments(m_episode, speechSegments);
  speechSegments = m_movieAnalyzer->denoiseSpeechSegments(speechSegments);
  speechSegments = m_movieAnalyzer->filterSpeechSegments(speechSegments);

  m_movieAnalyzer->coClustering(fName, shots, m_lsuShots, speechSegments, m_lsuSpeechSegments);

  return true;
}

bool ProjectModel::summarization(SummarizationDialog::Method method, int seasonNb, const QString &speaker, int dur, qreal granu)
{
  // retrieve scene speech segments for network building
  QList<QList<SpeechSegment *> > sceneSpeechSegments;
  m_scenes.clear();
  retrieveSpeakersNet_aux(m_series, sceneSpeechSegments, m_scenes);

  // retrieve LSU contents
  QList<QList<Shot *> > lsuShots;
  QList<QList<SpeechSegment *> > lsuSpeechSegments;
  
  // min and max LSU duration
  qint64 minDur(5000);
  qint64 maxDur(15000);
  
  retrieveLsuContents(m_series, lsuShots, lsuSpeechSegments, minDur, maxDur);

  m_movieAnalyzer->summarization(method, seasonNb, speaker, dur, granu, sceneSpeechSegments, lsuShots, lsuSpeechSegments);

  return true;
}

void ProjectModel::showNarrChart(bool checked)
{
  // retrieve scene speech segments for network building
  QList<QList<SpeechSegment *> > sceneSpeechSegments;
  m_scenes.clear();

  if (checked)
    retrieveSpeakersNet_aux(m_series, sceneSpeechSegments, m_scenes);

  emit viewNarrChart(sceneSpeechSegments);
}

void ProjectModel::extractShots(QString fName, int histoType, int nVBins, int nHBins, int nSBins, int metrics, qreal threshold1, qreal threshold2, int nVBlock, int nHBlock)
{
  removeAutoShots(m_episode);

  m_movieAnalyzer->extractShots(fName, histoType, nVBins, nHBins, nSBins, metrics, threshold1, threshold2, nVBlock, nHBlock);
  evaluateShotDetection(true, threshold1, threshold2);
}

void ProjectModel::labelSimilarShots(QString fName, int histoType, int nVBins, int nHBins, int nSBins, int metrics, qreal maxDist, int windowSize, int nVBlock, int nHBlock)
{
  resetAutoCameraLabels(m_episode);

  QList<Shot *> shots;
  retrieveShots(m_episode, shots);
  
  m_movieAnalyzer->labelSimilarShots(fName, histoType, nVBins, nHBins, nSBins, metrics, maxDist, windowSize, shots, nVBlock, nHBlock, true);
  evaluateSimShotDetection(true, maxDist, windowSize);
}

void ProjectModel::extractScenes(Segment::Source vSrc, const QString &fName)
{
  /***********************/
  /* retrieve audio data */
  /***********************/

  // matrix containing one i-vector/utterance
  mat X;
  X.load("spkDiarization/iv/X.dat");
  
  // covariance matrices
  mat Sigma;
  Sigma = cov(X);
  mat W;
  W.zeros(X.n_rows, X.n_cols);
  
  QMap<QString, QList<int> > spkIdx;
  for (int i(0); i < m_subBound.size(); i++)

    if (m_subRefLbl[i] != "S")
      spkIdx[m_subRefLbl[i]].push_back(i);
  
  W = genWMat(spkIdx, X);

  emit setDiarData(X, Sigma, W);


  /*****************************************/
  /* retrieving shots labels and positions */
  /*****************************************/
  
  QList<QString> shotLabels;
  retrieveShotLabels(m_series, shotLabels, vSrc);
  
  QList<qint64> shotPositions;
  retrieveShotPositions(m_series, shotPositions);


  /*************************/
  /* inserting first scene */
  /*************************/

  qint64 posInit(0);
  insertScene(posInit, Segment::Automatic);

  
  /*********************************/
  /* processing scene segmentation */
  /*********************************/

  m_movieAnalyzer->extractScenes(fName, shotLabels, shotPositions, m_subRefLbl, m_subBound);

  
  /**************/
  /* evaluation */
  /**************/

  evaluateSceneDetection(true);
}

void ProjectModel::faceDetection(const QString &fName, FaceDetectDialog::Method method, int minHeight)
{
  QList<Shot *> shots;
  retrieveShots(m_episode, shots);

  resetShotFaces(m_episode);

  switch (method) {
  case FaceDetectDialog::OpenCV:
    m_movieAnalyzer->faceDetectionOpenCV(shots, fName, minHeight);
    break;
  case FaceDetectDialog::Zhu:
    m_movieAnalyzer->faceDetectionZhu(shots, fName, minHeight);
    break;
  }
}

void ProjectModel::externFaceDetection(const QString &fName)
{
  resetShotFaces(m_episode);

  QMap<qint64, QList<QRect> > faceBound;

  QFile faceDetFile(fName);
  if (!faceDetFile.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QTextStream faceDetIn(&faceDetFile);
  while (!faceDetIn.atEnd()) {
    QString line = faceDetIn.readLine();
    QStringList fields = line.split("\t");
    qint64 pos = fields[0].toLong();
    int x = fields[1].toInt();
    int y = fields[2].toInt();
    int w = fields[3].toInt();
    int h = fields[4].toInt();
    faceBound[pos].push_back(QRect(x, y, w, h));
  }

  QMap<qint64, QList<QRect> >::const_iterator it = faceBound.begin();

  while (it != faceBound.end()) {

    qint64 position = it.key();
    setCurrShot(position);
    appendFaces(position, it.value());

    it++;
  }
}

qreal ProjectModel::evaluateShotDetection(bool displayResults, qreal thresh1, qreal thresh2) const
{
  QList<Segment *> scenes = m_episode->getChildren();
  qreal fps = m_episode->getFps();
  qint64 duration(0);
  int nVideoFrames(0);
  int tp(0);
  int fp(0);
  int tn(0);
  int fn(0);

  qreal precision;
  qreal recall;
  qreal fScore;
  qreal accuracy;

  for (int i(0); i < scenes.size(); i++) {
      
    for (int j(0); j < scenes[i]->childCount(); j++) {

      Segment *shot = scenes[i]->child(j);

      switch (shot->getSource()) {
      case Segment::Both:
	tp++;
	break;
      case Segment::Manual:
	fn++;
	break;
      case Segment::Automatic:
	fp++;
	break;
      }

      // update duration
      if (shot->getEnd() > duration)
	duration = shot->getEnd();
    }
  }

  nVideoFrames = static_cast<int>(duration / ((1000.0) / fps));
  tn = nVideoFrames - (tp + fp + fn);
  precision = computePrecision(tp, fp);
  recall = computeRecall(tp, fn);
  fScore = computeFScore(recall, precision);
  accuracy = computeAccuracy(tp, fp, fn, tn);

  if (displayResults) {
    ResultsDialog *dialog = new ResultsDialog(thresh1, thresh2, precision, recall, fScore, accuracy, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, tp, fp, fn, tn);
    dialog->exec();
  }

  return fScore;
}

qreal ProjectModel::evaluateSimShotDetection(bool displayResults, qreal thresh1, qreal thresh2) const
{
  int tp(0);
  int fp(0);
  int tn(0);
  int fn(0);

  qreal precision;
  qreal recall;
  qreal fScore;
  qreal accuracy;

  QList<int> autCamLabels;
  QList<int> manCamLabels;
  
  retrieveSimCamLabels(m_episode, autCamLabels, manCamLabels);

  for (int i(0); i < autCamLabels.size(); i++) {
    
    QList<int> manIdxSim;
    QList<int> autIdxSim;

    // lists of shots similar to current one, as manually and automatically annotated
    int j;
    for (j = 0; j < autCamLabels.size(); j++) {
      if (j != i && manCamLabels[i] == manCamLabels[j])
	manIdxSim.push_back(j);
      if (j != i && autCamLabels[i] == autCamLabels[j])
	autIdxSim.push_back(j);
    }

    if (manIdxSim.size() == 0 && autIdxSim.size() == 0)
      tn++;
    else if (manIdxSim.size() > 0 && autIdxSim.size() == 0)
      fn++;
    else if (manIdxSim.size() == 0 && autIdxSim.size() > 0)
      fp++;
    else {
      // test for emptiness of the intersection of the two lists
      bool found = false;

      for (int j(0); j < manIdxSim.size(); j++)
	if (autIdxSim.contains(manIdxSim[j]))
	  found = true;

      if (found)
	tp++;
      else
	fp++;
    }
  }

  precision = computePrecision(tp, fp);
  recall = computeRecall(tp, fn);
  fScore = computeFScore(recall, precision);
  accuracy = computeAccuracy(tp, fp, fn, tn);

  if (displayResults) {
    ResultsDialog *dialog = new ResultsDialog(thresh1, thresh2, precision, recall, fScore, accuracy, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, tp, fp, fn, tn);
    dialog->exec();
  }

  return fScore;
}

qreal ProjectModel::evaluateSceneDetection(bool displayResults) const
{
  Segment *season;
  Segment *episode;
  Segment *segment;

  int tp(0);
  int fp(0);
  int tn(0);
  int fn(0);
  int nScenes(0);
  int nShots(0);

  qreal precision;
  qreal recall;
  qreal fScore_1;
  qreal fScore_2;
  qreal accuracy;

  QList<int> ref_bound;
  QList<int> hyp_bound;

  for (int i(0); i < m_series->childCount(); i++) {
    season = m_series->child(i);

    for (int j(0); j < season->childCount(); j++) {
      episode = season->child(j);
      
      for (int k(0); k < episode->childCount(); k++) {
	segment = episode->child(k);

	// scene level exists
	if (dynamic_cast<Scene *>(segment)) {
	  nScenes++;
	  switch (segment->getSource()) {
	  case Segment::Both:
	    ref_bound.push_back(nShots);
	    hyp_bound.push_back(nShots);
	    tp++;
	    break;
	  case Segment::Manual:
	    ref_bound.push_back(nShots);
	    fn++;
	    break;
	  case Segment::Automatic:
	    hyp_bound.push_back(nShots);
	    fp++;
	    break;
	  }

	  // update shot index
	  nShots += segment->childCount();
	}
      }
    }
  }
  
  // decrement first shot
  nShots--;
  nScenes--;
  fn--;
  tn = nShots - (tp + fp + fn);

  precision = computePrecision(tp, fp);
  recall = computeRecall(tp, fn);
  fScore_1 = computeFScore(recall, precision);
  accuracy = computeAccuracy(tp, fp, fn, tn);

  int overflowIdx(0);
  int sceneSize(0);
  int prevSceneSize(0);
  int nextSceneSize(0);
  int j(0);
  int prevRefInf(0);
  int nextRefSup(0);
  int refInf(0);
  int refSup(0);
  int inter(0);
  int interMax(0);
  QList<int> lambda;
  qreal localCov(0.0);
  qreal coverage(0.0);
  qreal localOver(0.0);
  qreal overflow(0.0);
  int nLeftOver(0);
  int nRightOver(0);

  // compute coverage
  for (int i(0); i < ref_bound.size(); i++) {

    lambda.clear();

    prevRefInf = (i == 0) ? 0 : ref_bound[i-1];
    nextRefSup = (i < ref_bound.size() - 2) ? ref_bound[i+2] - 1 : nShots;
    refInf = ref_bound[i];
    refSup = (i < ref_bound.size() - 1) ? ref_bound[i+1] - 1 : nShots;

    sceneSize = refSup - refInf + 1;
    prevSceneSize = refInf - prevRefInf;
    nextSceneSize = nextRefSup - refSup;

    // set index of hypothesized LSUs list
    while (j >= 0 && hyp_bound[j] > refInf)
      j--;

    // move to hypothesized LSU overflowing left
    overflowIdx = (hyp_bound[j] < prevRefInf) ? prevRefInf : hyp_bound[j];
    nLeftOver = refInf - overflowIdx;

    // push first hypothesized LSU covered by current ref LSU
    lambda.push_back(refInf);

    // reset index of hypothesized LSUs
    while (j < hyp_bound.size() && hyp_bound[j] <= refInf)
      j++;

    // push next hypothesized LSUs covered by current ref LSU
    while (j < hyp_bound.size() && hyp_bound[j] <= refSup) {
      lambda.push_back(hyp_bound[j]);
      j++;
    }
    
    // move to hypothesized LSU overflowing right
    if (j == hyp_bound.size())
      overflowIdx = nShots;
    else
      overflowIdx = (hyp_bound[j] - 1 < nextRefSup) ? hyp_bound[j] - 1 : nextRefSup;

    nRightOver = overflowIdx - refSup;
    
    interMax = 0;

    for (int k = 1; k < lambda.size(); k++) {
      inter = lambda[k] - lambda[k-1];

      if (inter >= interMax)
	interMax = inter;
    }

    inter = refSup - lambda[lambda.size() - 1] + 1;
    if (inter >= interMax)
      interMax = inter;

    // compute local scores
    localOver = (static_cast<qreal>(nLeftOver) + nRightOver) / (prevSceneSize + nextSceneSize);
    localCov = static_cast<qreal>(interMax) / sceneSize;
    coverage += localCov * sceneSize;
    overflow += localOver * sceneSize;
  }

  coverage /= nShots;
  overflow /= nShots;

  fScore_2 = computeFScore(1 - overflow, coverage);

  if (displayResults) {
    ResultsDialog *dialog = new ResultsDialog(0, 0, precision, recall, fScore_1, accuracy, -1.0, -1.0, -1.0, coverage, overflow, fScore_2, tp, fp, fn, tn);
    dialog->exec();
  }

  return fScore_1;
}

qreal ProjectModel::computePrecision(int tp, int fp) const
{
  if (tp == 0 && fp == 0)
    return 0.0;

  return static_cast<qreal>(tp) / (tp + fp);
}

qreal ProjectModel::computeRecall(int tp, int fn) const
{
  if (tp == 0 && fn == 0)
    return 0.0;

  return static_cast<qreal>(tp) / (tp + fn);
}

qreal ProjectModel::computeFScore(qreal precision, qreal recall) const
{
  if (precision == 0.0 || recall == 0.0)
    return 0.0;

  return 2 * precision * recall / (precision + recall);
}

qreal ProjectModel::computeAccuracy(int tp, int fp, int fn, int tn) const
{
  return (static_cast<qreal>(tp) + tn) / (tp + fp + fn + tn);
}

void ProjectModel::resetAutoCameraLabels(Segment *segment)
{
  Shot *shot;

  if ((shot = dynamic_cast<Shot *>(segment)))
    shot->setCamera(-1, Segment::Automatic);
  
  else
    for (int i(0); i < segment->childCount(); i++)
      resetAutoCameraLabels(segment->child(i));
}

void ProjectModel::resetShotFaces(Segment *segment)
{
  Shot *shot;

  if ((shot = dynamic_cast<Shot *>(segment)))
    shot->clearFaces();
  
  else
    for (int i(0); i < segment->childCount(); i++)
      resetShotFaces(segment->child(i));
}

void ProjectModel::retrieveShotLabels(Segment *segment, QList<QString> &shotLabels, Segment::Source source) const
{
  if ((dynamic_cast<Shot *>(segment)))
    shotLabels.push_back(segment->getLabel(source));

  for (int i(0); i < segment->childCount(); i++)
    retrieveShotLabels(segment->child(i), shotLabels, source);
}


///////////////
// accessors //
///////////////

QString ProjectModel::getName() const
{
  return m_name;
}

QString ProjectModel::getBaseName() const
{
  return m_baseName;
}

QString ProjectModel::getSeriesName() const
{
  return m_series->getName();
}

///////////
// slots //
///////////

void ProjectModel::getSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots)
{
  emit grabSnapshots(snapshots);
}

void ProjectModel::setSpkDiarData(const arma::mat &X, const arma::mat &Sigma, const arma::mat &W)
{
  QList<SpeechSegment *> speechSegments;
  retrieveSpeechSegments(m_episode, speechSegments);
  speechSegments = m_movieAnalyzer->denoiseSpeechSegments(speechSegments);
  speechSegments = m_movieAnalyzer->filterSpeechSegments(speechSegments);
  
  emit initDiarData(X, Sigma, W, speechSegments);
}

void ProjectModel::insertMusicRate(QPair<qint64, qreal> musicRate)
{
  m_currShot->appendMusicRate(musicRate);
}

void ProjectModel::playSegments(QList<QPair<Episode *, QPair<qint64, qint64> > > segments)
{
  emit playLsus(segments);
}

void ProjectModel::setEpisode(Episode *currEpisode)
{
  m_episode = currEpisode;
  setLsuContents(Segment::Manual);
  emit resetSegmentView();
  emit resetSpkDiarMonitor();
}

void ProjectModel::retrieveLsuContents(Segment *segment, QList<QList<Shot *> > &lsuShots, QList<QList<SpeechSegment *> > &lsuSpeechSegments, qint64 minDur, qint64 maxDur)
{
  Episode *episode;

  if ((episode = dynamic_cast<Episode *>(segment))) {

    qDebug() << episode->getFName();

    QList<QList<Shot *> > episodeLsuShots;
    setLsuShots(episode, episodeLsuShots, Segment::Automatic, minDur, maxDur, true, true);

    QList<QList<SpeechSegment *> > episodeLsuSpeechSegments;
    setLsuSpeechSegments(episode, episodeLsuSpeechSegments, episodeLsuShots);

    lsuShots.append(episodeLsuShots);
    lsuSpeechSegments.append(episodeLsuSpeechSegments);
  }

  else
    for (int i(0); i < segment->childCount(); i++)
      retrieveLsuContents(segment->child(i), lsuShots, lsuSpeechSegments, minDur, maxDur);
}

void ProjectModel::setLsuContents(Segment::Source source)
{
  setLsuShots(m_episode, m_lsuShots, source);
  setLsuSpeechSegments(m_episode, m_lsuSpeechSegments, m_lsuShots);
}

void ProjectModel::setLsuShots(Episode *episode, QList<QList<Shot *> > &lsuShots, Segment::Source source, qint64 minDur, qint64 maxDur, bool rec, bool sceneBound)
{
  QList<Shot *> shots;
  retrieveShots(episode, shots);

  lsuShots.clear();

  if (shots.size() > 0)
    lsuShots = m_movieAnalyzer->retrieveLsuShots(shots, source, minDur, maxDur, rec, sceneBound);
}

void ProjectModel::setLsuSpeechSegments(Episode *episode, QList<QList<SpeechSegment *> > &lsuSpeechSegments, QList<QList<Shot *> > &lsuShots)
{
  QList<Shot *> shots;
  retrieveShots(episode, shots);

  lsuSpeechSegments.clear();

  QList<SpeechSegment *> speechSegments;
  retrieveSpeechSegments(episode, speechSegments);
  speechSegments = m_movieAnalyzer->denoiseSpeechSegments(speechSegments);
  speechSegments = m_movieAnalyzer->filterSpeechSegments(speechSegments);

  QList<QList<SpeechSegment *> > shotSpeechSegments = m_movieAnalyzer->retrieveShotSpeechSegments(shots, speechSegments);

  for (int i(0); i < lsuShots.size(); i++) {

    QList<SpeechSegment *> currLsuSpeechSegments;
    
    for (int j(0); j < lsuShots[i].size(); j++) {
      int k = shots.indexOf(lsuShots[i][j]);
      currLsuSpeechSegments.append(shotSpeechSegments[k]);
    }
    
    lsuSpeechSegments.push_back(currLsuSpeechSegments);
  }
}

void ProjectModel::initShotAnnot(bool checked)
{
  if (checked) {
    setShotsToManual(m_episode);
    insertVideoFrames(m_episode);
    emit setDepthView(getDepth());
  }
  else {
    removeVideoFrames(m_episode);
    emit setDepthView(getDepth());
  }
}

void ProjectModel::setShotsToManual(Segment *segment)
{
  if ((dynamic_cast<Shot *>(segment)))
    segment->setSource(Segment::Manual);
  else
    for (int i(0); i < segment->childCount(); i++)
      setShotsToManual(segment->child(i));
}

void ProjectModel::insertVideoFrames(Episode *currEpisode)
{
  qreal fps = currEpisode->getFps();
  insertVideoFrames_aux(m_episode, fps);
}

void ProjectModel::insertVideoFrames_aux(Segment *segment, qreal fps)
{
  if ((dynamic_cast<Shot *>(segment))) {

    qint64 start = segment->getPosition();
    qint64 end = segment->getEnd();
    qreal step = 1000.0 / fps;
    int nFrames = static_cast<int>((end - start) / step);

    QList<Segment *> segmentsToInsert;

    // video frames to insert
    for (int i(0); i < nFrames; i++) {
      int id = i + 1;
      qint64 position = start + i * step;
      segmentsToInsert.push_back(new VideoFrame(id,
						position,
						segment));
    }

    setSegmentsToInsert(segmentsToInsert);
    
    // index of shot within model
    QModelIndex parent = indexFromSegment(segment);
    
    // insert video frames
    insertRows(0, segmentsToInsert.size(), parent);
  }
  
  else
    for (int i(0); i < segment->childCount(); i++)
      insertVideoFrames_aux(segment->child(i), fps);
}

void ProjectModel::removeVideoFrames(Segment *segment)
{
  if ((dynamic_cast<Shot *>(segment))) {

    // video frames to remove
    QList<Segment *> segmentsToRemove = segment->getChildren();

    // index of shot within model
    QModelIndex parent = indexFromSegment(segment);
    
    // remove video frames
    removeRows(0, segmentsToRemove.size(), parent);
  }
  
  else
    for (int i(0); i < segment->childCount(); i++)
      removeVideoFrames(segment->child(i));
}

void ProjectModel::insertShotAuto(qint64 position, qint64 end)
{
  // retrieve insertion scene
  int i = m_episode->childIndexFromPosition(position);
  Segment *scene = m_episode->child(i);

  // retrieve index closest shot to position
  int iCloseShot = scene->childIndexFromPosition(position);
  Segment *closeShot;

  // shot exists
  if (iCloseShot != -1 && (closeShot = scene->child(iCloseShot))->getPosition() == position)
    closeShot->setSource(Segment::Both);

  // shot does not exist: insert it
  else {
    // insert shot after closest one
    Shot *shot = new Shot(position, Shot::Cut, scene, Segment::Automatic);
    shot->setEnd(end);

    // update view
    QList<Segment *> segmentsToInsert;
    segmentsToInsert.push_back(shot);
    setSegmentsToInsert(segmentsToInsert);
    QModelIndex parent = indexFromSegment(scene);
    insertRows(iCloseShot + 1, segmentsToInsert.size(), parent);
  }
  
  emit resetSegmentView();
  emit setDepthView(getDepth());
  emit positionChanged(position);
}

void ProjectModel::appendFaces(qint64 position, const QList<QRect> &faces)
{
  m_currShot->appendFaces(position, faces);
}

void ProjectModel::insertScene(qint64 position, Segment::Source source)
{
  int i(-1);

  // retrieve shot from the position specified
  Segment *segment = m_series;

  while (!dynamic_cast<Shot *>(segment)) {

    // closest segment index to current position
    i = segment->childIndexFromPosition(position);

    // select possible children
    segment = segment->child(i);
  }

  insertSegment(segment, source);
}

void ProjectModel::insertSegment(Segment *segment, Segment::Source source)
{
  QList<Segment *> segmentsToInsert;

  Segment *prevParent = segment->parent();
  Segment *grandParent = prevParent->parent();
      
  // no segment already added at this position
  if (segment->getPosition() != prevParent->getPosition()) {

    Segment *newParent = 0;
    
    if ((dynamic_cast<VideoFrame *>(segment))) {
      newParent = new Shot(segment->getPosition(), Shot::Cut, grandParent, source);
      Shot *prevShot = dynamic_cast<Shot *>(prevParent);
      newParent->setEnd(prevShot->getEnd());
      prevShot->setEnd(newParent->getPosition());
    }
    else
      newParent = new Scene(segment->getPosition(), grandParent, source);

    QList<Segment *> subList1;
    QList<Segment *> subList2;

    prevParent->splitChildren(subList1, subList2, segment->row(), newParent);
    prevParent->clearChildren();
    prevParent->setChildren(subList1);
    newParent->setChildren(subList2);

    // inserting new segment
    segmentsToInsert.push_back(newParent);
    setSegmentsToInsert(segmentsToInsert);
    QModelIndex parent = indexFromSegment(grandParent);
    insertRows(prevParent->row() + 1, segmentsToInsert.size(), parent);

    emit positionChanged(segment->getPosition());
    emit resetSegmentView();
  }
}

void ProjectModel::removeSegment(Segment *segment)
{
  // row of current segment
  int row = segment->row();

  // parent of segment to remove
  Segment *parentSegment = segment->parent();

  // segment to delete must not be the first one
  if (row != 0) {

    // previous segment
    Segment *prevSegment = parentSegment->child(row - 1);
    
    // segments to move
    QList<Segment *> segmentChildren = segment->getChildren();
    QList<Segment *> segmentsToInsert;
    
    // copying children of removed segment
    for (int i = 0; i < segmentChildren.size(); i++) {
      Segment *child = 0;
      if ((dynamic_cast<Shot *>(segment))) {
	child = new VideoFrame(i + 1,
			       segmentChildren[i]->getPosition(),
			       prevSegment);
	Shot *prevShot = dynamic_cast<Shot *>(prevSegment);
	Shot *shot = dynamic_cast<Shot *>(segment);
	prevShot->setEnd(shot->getEnd());
      }
      else {
	child = new Shot(segmentChildren[i]->getPosition(),
			 Shot::Cut,
			 prevSegment,
			 segmentChildren[i]->getSource());
	child->setEnd(segmentChildren[i]->getEnd());
      }
      segmentsToInsert.push_back(child);
    }

    // removing segment
    QModelIndex parent = indexFromSegment(parentSegment);
    removeRows(row, 1, parent);

    // inserting children at their new place
    setSegmentsToInsert(segmentsToInsert);
    parent = indexFromSegment(prevSegment);
    insertRows(prevSegment->childCount(), segmentsToInsert.size(), parent);

    emit positionChanged(prevSegment->getPosition());
    emit resetSegmentView();
  }
}

void ProjectModel::processSegmentation(bool autoChecked, bool refChecked, bool annot)
{
  bool view(false);

  if (refChecked || autoChecked || annot) {

    // retrieving shots
    QList<Shot *> shots;
    retrieveShots(m_episode, shots);

    QList<Segment *> shSegments;
    int n(0);
    for (int i(0); i < shots.size(); i++) {
      n += (shots[i]->getMusicRates()).size();
      shSegments.push_back(shots[i]);
    }

    // retrieving speech segments
    QList<SpeechSegment *> speechSegments;
    retrieveSpeechSegments(m_episode, speechSegments);
    speechSegments = m_movieAnalyzer->denoiseSpeechSegments(speechSegments);
    // speechSegments = m_movieAnalyzer->filterSpeechSegments(speechSegments);
    QList<Segment *> spSegments;
    for (int i(0); i < speechSegments.size(); i++)
      spSegments.push_back(speechSegments[i]);

    // retrieving reference speakers
    QStringList refSpeakers = retrieveRefSpeakers();
    emit getRefSpeakers(refSpeakers);

    if (annot || refChecked) {
      emit getShots(shSegments, Segment::Manual);
      emit getSpeechSegments(spSegments, Segment::Manual);
    }

    else if (autoChecked) {
      emit getShots(shSegments, Segment::Automatic);
      emit getSpeechSegments(spSegments, Segment::Manual);
    }

    view = true;
  }
  
  emit viewSegmentation(view, annot);
}

QStringList ProjectModel::retrieveRefSpeakers()
{
  QMap<QString, qreal> refSpeakers;
  QMap<qreal, QString> invRefSpeakers;
  QStringList speakers;
  QStringList revSpeakers;

  retrieveRefSpeakers_aux(m_series, refSpeakers);

  QMap<QString, qreal>::const_iterator it = refSpeakers.begin();

  while (it != refSpeakers.end()) {
    QString refSpeaker = it.key();
    qreal weight = it.value();
    invRefSpeakers[weight] = refSpeaker;

    it++;
  }

  speakers = invRefSpeakers.values();
  for (int i(0); i < speakers.size(); i++)
    revSpeakers.push_front(speakers[i]);

  return revSpeakers;
}

void ProjectModel::retrieveRefSpeakers_aux(Segment *segment, QMap<QString, qreal> &refSpeakers)
{
  Episode *episode;
  
  if ((episode = dynamic_cast<Episode *>(segment))) {
    QList<SpeechSegment *> speechSegments;
    retrieveSpeechSegments(episode, speechSegments);
    for (int i(0); i < speechSegments.size(); i++) {
      qreal duration = (speechSegments[i]->getEnd() - speechSegments[i]->getPosition()) / 1000.0;
      QString refSpeaker = speechSegments[i]->getLabel(Segment::Manual);
      if (refSpeaker != "" && refSpeaker != "S")
	refSpeakers[refSpeaker] += duration;
    }
  }
  else
    for (int i(0); i < segment->childCount(); i++)
      retrieveRefSpeakers_aux(segment->child(i), refSpeakers);
}
 
void ProjectModel::retrieveSpeakersNet(Segment *segment)
{
  QList<QList<SpeechSegment *> > sceneSpeechSegments;
  m_scenes.clear();

  // retrieveSpeakersNet_aux(segment, sceneSpeechSegments, scenes);
  // retrieve scene speech segments for network building
  retrieveSpeakersNet_aux(m_series, sceneSpeechSegments, m_scenes);
  if (!sceneSpeechSegments.isEmpty())
    emit viewSpkNet(sceneSpeechSegments);

  // request view corresponding to selected scene
  updateSpeakersNet(segment);
}

void ProjectModel::updateSpeakersNet(Segment *segment)
{
  Scene *currScene(0);
  Segment *tmp = segment;

  if (tmp == 0)
    tmp = m_episode;

  // retrieve closest scene to selection
  while (tmp != 0 && !(currScene = dynamic_cast<Scene *>(tmp)))
    tmp = tmp->child(0);

  if (!tmp) {
    tmp = segment;
    while (tmp != 0 && !(currScene = dynamic_cast<Scene *>(tmp)))
      tmp = tmp->parent();
  }

  // retrieve scene index
  int iScene = m_scenes.indexOf(currScene);

  if (iScene != -1)
    emit updateSpkView(iScene);
}

void ProjectModel::retrieveSpeakersNet_aux(Segment *segment, QList<QList<SpeechSegment *> > &sceneSpeechSegments, QList<Scene *> &scenes, QVector<QPair<int, int> > sel, bool lsu)
{
  Episode *episode;

  if ((episode = dynamic_cast<Episode *>(segment))) {

    // retrieve season/episode number
    int seasNbr = episode->parent()->getNumber();
    int epNbr = episode->getNumber();

    QPair<int, int> currEp(seasNbr, epNbr);

    if (sel.isEmpty() || sel.contains(currEp)) {

      // retrieving speech segments/shot
      QList<Shot *> shotsEpisode;
      retrieveShots(episode, shotsEpisode);
      
      QList<SpeechSegment *> speechSegmentsEpisode;
      retrieveSpeechSegments(episode, speechSegmentsEpisode);
      speechSegmentsEpisode = m_movieAnalyzer->denoiseSpeechSegments(speechSegmentsEpisode);
      speechSegmentsEpisode = m_movieAnalyzer->filterSpeechSegments(speechSegmentsEpisode);
    
      QList<QList<SpeechSegment *> > shotSpeechSegmentsEpisode = m_movieAnalyzer->retrieveShotSpeechSegments(shotsEpisode, speechSegmentsEpisode);

      // gathering speech segments / scene
      QList<QList<SpeechSegment *> > sceneSpeechSegmentsEpisode;

      // retrieve speech segments located in Logical Story Units
      if (lsu) {

	QList<QList<Shot *> > lsuShotsEpisode;
	// lsuShotsEpisode = m_movieAnalyzer->retrieveLsuShots(shotsEpisode, Segment::Manual);

	for (int i(0); i < lsuShotsEpisode.size(); i++) {
	  for (int j(0); j < lsuShotsEpisode[i].size(); j++) {

	    int shotIdx = shotsEpisode.indexOf(lsuShotsEpisode[i][j]);
	    QList<SpeechSegment *> lsuSpeechSegments = shotSpeechSegmentsEpisode[shotIdx];
	    sceneSpeechSegmentsEpisode.push_back(lsuSpeechSegments);
	  }
	}
      }

      // retrieve speech segments located in Scenes
      else {
	QList<SpeechSegment *> currSceneSpeechSegments;
	Scene *currScene;

	for (int i(0); i < shotSpeechSegmentsEpisode.size(); i++) {

	  // current scene
	  currScene = dynamic_cast<Scene *>(shotsEpisode[i]->parent());

	  // new scene
	  if (shotsEpisode[i]->row() == 0 && !currSceneSpeechSegments.isEmpty()) {

	    sceneSpeechSegmentsEpisode.push_back(currSceneSpeechSegments);
	    scenes.push_back(currScene);
	    
	    currSceneSpeechSegments.clear();
	  }
	  
	  // add current shot speech segments to current scene
	  currSceneSpeechSegments.append(shotSpeechSegmentsEpisode[i]);
	}

	sceneSpeechSegmentsEpisode.push_back(currSceneSpeechSegments);
	scenes.push_back(currScene);
      }

      sceneSpeechSegments.append(sceneSpeechSegmentsEpisode);
      
    }
  }

  else
    for (int i(0); i < segment->childCount(); i++)
      retrieveSpeakersNet_aux(segment->child(i), sceneSpeechSegments, scenes, sel, lsu);
}

void ProjectModel::retrieveEpisodeNetworks(Segment *segment, QList<QMap<QString, QMap<QString, qreal> > > &networks)
{
  Episode *episode;

  if ((episode = dynamic_cast<Episode *>(segment))) {
    QList<QList<SpeechSegment *> > sceneSpeechSegments;
    QList<Scene *> scenes;
    retrieveSpeakersNet_aux(episode, sceneSpeechSegments, scenes);
    QMap<QString, QMap<QString, qreal> > net = getUttOrientedNet(sceneSpeechSegments, SpkInteractDialog::Sequential);
    if (!net.isEmpty())
      networks.push_back(net);
  }
  else
    for (int i(0); i < segment->childCount(); i++)
      retrieveEpisodeNetworks(segment->child(i), networks);
}

void ProjectModel::retrieveSceneNetworks(Segment *segment, QList<QMap<QString, QMap<QString, qreal> > > &networks)
{
  Episode *episode;

  if ((episode = dynamic_cast<Episode *>(segment))) {
    QList<QList<SpeechSegment *> > sceneSpeechSegments;
    QList<Scene *> scenes;
    retrieveSpeakersNet_aux(episode, sceneSpeechSegments, scenes);
    for (int i(0); i < sceneSpeechSegments.size(); i++) {
      QList<QList<SpeechSegment *> > currSceneSpeechSeg;
      currSceneSpeechSeg.push_back(sceneSpeechSegments[i]);
      QMap<QString, QMap<QString, qreal> > net = getUttOrientedNet(currSceneSpeechSeg, SpkInteractDialog::Sequential);
      // QMap<QString, QMap<QString, qreal> > net = getSpkOrientedNet(currSceneSpeechSeg, SpkInteractDialog::CoOccurr);
      if (!net.isEmpty())
	networks.push_back(net);
    }
  }
  else
    for (int i(0); i < segment->childCount(); i++)
      retrieveSceneNetworks(segment->child(i), networks);
}

void ProjectModel::exportShotBound(const QString &fName)
{
  const int NSAMPLES(4);
  int minDur = static_cast<int>(1000.0 / m_episode->getFps());

  QFile shotFile(fName + ".csv");

  if (!shotFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QTextStream shotOut(&shotFile);

  QList<Shot *> shots;
  retrieveShots(m_episode, shots);

  for (int i(0); i < shots.size(); i++) {

    qint64 first = shots[i]->getPosition();
    qint64 last = shots[i]->getEnd() - minDur;
    int duration = last - first;
    qreal step(0.0);

    if (NSAMPLES > 0)
      step = static_cast<qreal>(duration) / NSAMPLES;

    QList<qint64> samplePos;
    samplePos.push_back(first);

    for (int j(1); j <= NSAMPLES; j++) {
      qint64 pos = static_cast<qint64>(qRound((first + j * step) / minDur) * minDur);
      if (pos > samplePos.last())
	samplePos.push_back(pos);
    }

    for (int j(0); j < samplePos.size(); j++)
      shotOut << samplePos[j] << endl;
  }
}

void ProjectModel::setCurrSpeechSeg(qint64 position)
{
  QList<SpeechSegment *> speechSegments;
  retrieveSpeechSegments(m_episode, speechSegments);
  bool found(false);
  int iSup(speechSegments.size()-1);
  int iInf(0);
  int iMed(-1);
  
  while (!found && iSup >= iInf) {
    iMed = (iSup + iInf) / 2;
    if (position >= speechSegments[iMed]->getPosition() && position <= speechSegments[iMed]->getEnd())
      found = true;
    else if (position < speechSegments[iMed]->getPosition())
      iSup = iMed - 1;
    else if (position > speechSegments[iMed]->getEnd())
      iInf = iMed + 1;
  }

  // new speech segment
  if (found) {
    if (m_currSpeechSeg != speechSegments[iMed]) {
      m_currSpeechSeg = speechSegments[iMed];
      emit displaySubtitle(m_currSpeechSeg);
    }
  }
  // no speech segment
  else {
    if (m_currSpeechSeg != 0) {
      m_currSpeechSeg = 0;
      emit displaySubtitle(new SpeechSegment(0, 0, "", QVector<QString>(), QVector<QStringList>()));
    }
  }
}

void ProjectModel::setCurrShot(qint64 position)
{
  QList<Shot *> shots;
  retrieveShots(m_episode, shots);
  bool found(false);
  int iSup(shots.size()-1);
  int iInf(0);
  int iMed(-1);
  
  while (!found && iSup >= iInf) {
    iMed = (iSup + iInf) / 2;
    if (position >= shots[iMed]->getPosition() && position < shots[iMed]->getEnd())
      found = true;
    else if (position < shots[iMed]->getPosition())
      iSup = iMed - 1;
    else if (position >= shots[iMed]->getEnd())
      iInf = iMed + 1;
  }

  // possibly update current shot
  if (m_currShot != shots[iMed])
    m_currShot = shots[iMed];
}

void ProjectModel::getCurrFaces(qint64 position)
{
  QList<QPair<qint64, QList<Face> > > faces = m_currShot->getFaces();
  qreal fps = m_episode->getFps();
  qreal step = (1.0 / fps) * 1000;

  position = qRound(position / step) * step;
  
  bool found(false);
  int iSup(faces.size() - 1);
  int iInf(0);
  int iMed(-1);

  while (!found && iSup >= iInf) {
    iMed = static_cast<int>(iSup + iInf) / 2;
    if (faces[iMed].first == position)
      found = true;
    else if (position < faces[iMed].first)
      iSup = iMed - 1;
    else if (position > faces[iMed].first)
      iInf = iMed + 1;
  }
  
  // possibly update current shot
  if (found)
    emit displayFaces(faces[iMed].second);
  else
    emit displayFaces(QList<Face>());
}

void ProjectModel::getCurrLSU(qint64 position)
{
  // retrieving current LSU by using binary seach
  bool found(false);
  int iSup(m_lsuShots.size() - 1);
  int iInf(0);
  int iMed(-1);

  while (!found && iSup >= iInf) {
    iMed = (iSup + iInf) / 2;
    if (position >= m_lsuShots[iMed].first()->getPosition() && position <= m_lsuShots[iMed].last()->getEnd())
      found = true;
    else if (position < m_lsuShots[iMed].first()->getPosition())
      iSup = iMed - 1;
    else if (position > m_lsuShots[iMed].last()->getEnd())
      iInf = iMed + 1;
  }

  // current LSU speech segments
  QPair<int, int> lsuSpeechBound;

  // position within LSU
  if (found) {

    QList<SpeechSegment *> speechSegments;
    retrieveSpeechSegments(m_episode, speechSegments);
    speechSegments = m_movieAnalyzer->denoiseSpeechSegments(speechSegments);
    speechSegments = m_movieAnalyzer->filterSpeechSegments(speechSegments);

    for (int j(0); j < m_lsuSpeechSegments[iMed].size(); j++) {
      int first = speechSegments.indexOf(m_lsuSpeechSegments[iMed].first());
      int last = speechSegments.indexOf(m_lsuSpeechSegments[iMed].last());
      lsuSpeechBound.first = first;
      lsuSpeechBound.second = last;
    }
  }

  emit getCurrentPattern(lsuSpeechBound);
}

void ProjectModel::retrieveShots(Segment *segment, QList<Shot *> &shots)
{
  Shot *shot;
  
  if ((shot = dynamic_cast<Shot *>(segment)))
    shots.push_back(shot);

  for (int i(0); i < segment->childCount(); i++)
    retrieveShots(segment->child(i), shots);
}

QStringList ProjectModel::getSeasons()
{
  QStringList seasons;

  getSeasons_aux(m_series, seasons);

  return seasons;
}

void ProjectModel::getSeasons_aux(Segment *segment, QStringList &seasons)
{
  Season *season;

  if ((season = dynamic_cast<Season *>(segment)))
    seasons.push_back("Seasons 1 -> " + QString::number(season->getNumber()));

  for (int i(0); i < segment->childCount(); i++)
    getSeasons_aux(segment->child(i), seasons);
}

QList<QStringList> ProjectModel::getSeasonSpeakers()
{
  QList<QStringList> seasonSpeakers;

  getSeasonSpeakers_aux(m_series, seasonSpeakers);

  return seasonSpeakers;
}

void ProjectModel::getSeasonSpeakers_aux(Segment *segment, QList<QStringList> &seasonSpeakers)
{
  if ((dynamic_cast<Season *>(segment))) {

    QMap<QString, qreal> refSpeakers;
    QMap<qreal, QString> invRefSpeakers;
    QStringList speakers;
    QStringList revSpeakers;

    retrieveRefSpeakers_aux(segment, refSpeakers);

    QMap<QString, qreal>::const_iterator it = refSpeakers.begin();

    while (it != refSpeakers.end()) {
      QString refSpeaker = it.key();
      qreal weight = it.value();
      invRefSpeakers[weight] = refSpeaker;

      it++;
    }

    speakers = invRefSpeakers.values();
    for (int i(0); i < speakers.size(); i++)
      revSpeakers.push_front(speakers[i]);

    seasonSpeakers.push_back(revSpeakers);
  }

  for (int i(0); i < segment->childCount(); i++)
    getSeasonSpeakers_aux(segment->child(i), seasonSpeakers);
}

void ProjectModel::retrieveSpeechSegments(Episode *episode, QList<SpeechSegment *> &speechSegments)
{
  speechSegments = episode->getSpeechSegments();
}

void ProjectModel::removeAutoShots(Segment *segment)
{
  if ((dynamic_cast<Scene *>(segment))) {

    QList<Segment *> children = segment->getChildren();
    
    for (int i(segment->childCount() - 1); i >= 0; i--)

      if (children[i]->getSource() == Segment::Automatic) {
	QModelIndex parent = indexFromSegment(segment);
	removeRows(i, 1, parent);
      }

      else if (children[i]->getSource() == Segment::Both)
	children[i]->setSource(Segment::Manual);
  }

  for (int i(0); i < segment->childCount(); i++)
    removeAutoShots(segment->child(i));
}


void ProjectModel::retrieveShotPositions(Segment *segment, QList<qint64> &shotPositions) const
{
  if ((dynamic_cast<Shot *>(segment)))
    shotPositions.push_back(segment->getPosition());

  for (int i(0); i < segment->childCount(); i++)
    retrieveShotPositions(segment->child(i), shotPositions);
}

void ProjectModel::retrieveScenePositions(Segment *segment, QList<qint64> &scenePositions) const
{
  if ((dynamic_cast<Scene *>(segment)))
    scenePositions.push_back(segment->getPosition());

  for (int i(0); i < segment->childCount(); i++)
    retrieveScenePositions(segment->child(i), scenePositions);
}

void ProjectModel::retrieveShotBound(Segment *segment, QList<QPair<int, int> > &shotBound) const
{
  if ((dynamic_cast<Shot *>(segment))) {
 
    VideoFrame *firstFrame = dynamic_cast<VideoFrame *>(segment->child(0));
    int id = firstFrame->getId();

    shotBound.push_back(QPair<int, int>(id, -1));
  }

  for (int i(0); i < segment->childCount(); i++) {

    if ((dynamic_cast<VideoFrame *>(segment->child(i))) && i == segment->childCount() - 1) {

      VideoFrame *lastFrame = dynamic_cast<VideoFrame *>(segment->child(i));
      int id = lastFrame->getId();
      
      shotBound[shotBound.size() - 1].second = id;
    }


    retrieveShotBound(segment->child(i), shotBound);
  }
}

void ProjectModel::retrieveSimCamLabels(Segment *segment, QList<int> &autCamLabels, QList<int> &manCamLabels) const
{
  if ((dynamic_cast<Shot *>(segment))) {
    autCamLabels.push_back(segment->getCamera(Segment::Automatic));
    manCamLabels.push_back(segment->getCamera(Segment::Manual));
  }

  for (int i(0); i < segment->childCount(); i++)
    retrieveSimCamLabels(segment->child(i), autCamLabels, manCamLabels);
}

arma::mat ProjectModel::genWMat(QMap<QString, QList<int>> spkIdx, const arma::mat &X)
{
  // computing W
  mat W;

  if (spkIdx.size() > 0) {
    
    int nUtter(m_subBound.size());
    umat spkRows = zeros<umat>(spkIdx.size(), nUtter);

    QMap<QString, QList<int>>::const_iterator it = spkIdx.begin();
  
    int i(0);

    while (it != spkIdx.end()) {

      QList<int> indices = it.value();

      for (int j(0); j < indices.size(); j++)
	spkRows(i, indices[j]) = 1;

      it++;
      i++;
    }

    int m(X.n_rows);
    int n(X.n_cols);
    int nSpk(spkRows.n_rows);
  
    W.zeros(n, n);

    // looping over the speakers
    for (int i(0); i < nSpk; i++) {
    
      // index of current speaker utterances in X matrix
      umat idx = find(spkRows.row(i));
    
      // number of current speaker utterances
      int nUtt(idx.n_rows);

      // submatrix of vectorized speaker utterances
      mat S = X.rows(idx);

      // speaker covariance matrix
      mat C = zeros(n, n);

      // mean vector of speaker utterance vectors
      mat mu = mean(S);

      // looping over speaker utterances
      for (int j(0); j < nUtt; j++) {
	mat dev = S.row(j) - mu;
	C += dev.t() * dev;
      }

      // updating W
      W += C;
    }

    // normalizing W
    W /= m;
  }
  
  else
    W = cov(X);

  return W;
}
