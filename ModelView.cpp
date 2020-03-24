#include <QVBoxLayout>

#include "ModelView.h"
#include "Scene.h"
#include "Shot.h"
#include "VideoFrame.h"

#include <QDebug>

ModelView::ModelView(ProjectModel *project, int nVignettes, QWidget *parent)
  : QWidget(parent),
    m_currSegment(0),
    m_currEpisode(0),
    m_project(project),
    m_nVignettes(nVignettes),
    m_playerPaused(true)
{
  m_treeView = new QTreeView;
  m_treeView->expandAll();

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(m_treeView);
  setLayout(layout);

  m_treeView->setModel(m_project);
  m_selection = m_treeView->selectionModel();

  connect(m_selection, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(updatePlayer(const QModelIndex &, const QModelIndex &)));
  connect(m_selection, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(updateCurrentSegment(const QModelIndex &, const QModelIndex &)));
}

void ModelView::adjust()
{
  m_treeView->resizeColumnToContents(0);
  m_treeView->resizeColumnToContents(1);
}

void ModelView::initPlayer()
{
  QModelIndex seasonIndex = m_project->index(0, 0);
  updatePlayer(seasonIndex, QModelIndex());
}

void ModelView::initEpisodes(QList<Episode *> episodes)
{
  m_episodes = episodes;
}

void ModelView::insertEpisode(int index, Episode *episode)
{
  m_episodes.insert(index, episode);
}

/////////////////////////////////////////////
// slot called when clicking on model view //
/////////////////////////////////////////////

void ModelView::updatePlayer(const QModelIndex &current, const QModelIndex &previous)
{
  Q_UNUSED(previous);

  // scroll to selected segment
  m_treeView->scrollTo(current, QAbstractItemView::PositionAtCenter);

  // selected segment
  Segment *segment = static_cast<Segment *>(current.internalPointer());

  // position of selected segment
  qint64 position = segment->getPosition();

  // fetch current episode
  updateCurrentEpisode(segment);

  // update player position
  emit setPlayerPosition(position);

  // generating surrounding shot positions if a shot is selected
  if (dynamic_cast<Shot *>(segment))
    emit currentShot(genPositionList(segment));
  
  // update current speech segment
  if (m_playerPaused)
    emit newManPosition(position);
}

///////////////////////////////////////////
// slot called when player state changes //
///////////////////////////////////////////

void ModelView::playerPaused(bool pause)
{
  m_playerPaused = pause;
}

//////////////////////////////////////////////
// slot called when player position changes //
//////////////////////////////////////////////

void ModelView::positionChanged(qint64 position)
{
  int i(0);
  QModelIndex index;
  QModelIndex siblingIndex;

  m_selection->clearSelection();

  Segment *segment = m_currEpisode;

  // retrieve segment to select
  int depth = 0;
  while (depth++ < m_depth) {

    // closest segment index to current player position
    i = segment->childIndexFromPosition(position);

    // select possible children
    segment = segment->child(i);
  }

  // model indexes to select
  index = m_project->indexFromSegment(segment);
  siblingIndex = m_project->sibling(segment->row(), 1, index);
  
  // segment selection
  m_selection->setCurrentIndex(index, QItemSelectionModel::Select);
  m_selection->setCurrentIndex(siblingIndex, QItemSelectionModel::Select);
}

////////////////////////////////////////////
// slot called when episode index changes //
////////////////////////////////////////////

void ModelView::episodeIndexChanged(int index)
{
  updateCurrentEpisode(m_episodes[index]);
}

///////////////////////////////////
// slot called when pressing key //
///////////////////////////////////

void ModelView::keyPressEvent(QKeyEvent *event)
{
  Segment *segment = static_cast<Segment *>(m_selection->currentIndex().internalPointer());

  if (event->key() == Qt::Key_Return)
    emit returnPressed(segment);
  else if (event->key() == Qt::Key_Backspace)
    emit delPressed(segment);
}

////////////////////////////////
// slot called to expand view //
////////////////////////////////

void ModelView::setDepth(int depth)
{
  m_depth = depth - 2;
  if (m_depth == -1)
    m_treeView->collapseAll();
  else
    m_treeView->expandToDepth(m_depth);
  adjust();
}

//////////////////////////////////////////////
// slot called for updating current episode //
//////////////////////////////////////////////

void ModelView::updateEpisode(Episode *episode)
{
  updateCurrentEpisode(episode);
}

//////////////////////////////////////////////
// slot called for updating current segment //
//////////////////////////////////////////////

void ModelView::updateCurrentSegment(const QModelIndex &current, const QModelIndex &previous)
{
  Q_UNUSED(previous);

  // selected segment
  Segment *segment = static_cast<Segment *>(current.internalPointer());

  m_currSegment = segment;
  emit resetSpeakersView();
}

///////////////
// accessors //
///////////////

QString ModelView::getCurrentEpisodeFName() const
{
  return m_currEpisode->getFName();
}

QString ModelView::getCurrentEpisodeNumber() const
{
  QString epNumberStr;
  QString seasNumberStr;
  QString epRank;

  int epNumber = m_currEpisode->getNumber();
  int seasNumber = m_currEpisode->parent()->getNumber();
  
  epNumberStr = QString::number(epNumber);
  if (epNumber < 10)
    epNumberStr = "0" + epNumberStr;

  seasNumberStr = QString::number(seasNumber);
  if (seasNumber < 10)
    seasNumberStr = "0" + seasNumberStr;


  return "S" + seasNumberStr + "E" + epNumberStr;
}

QSize ModelView::getCurrentEpisodeRes() const
{
  return m_currEpisode->getResolution();
}

Episode * ModelView::getCurrentEpisode()
{
  return m_currEpisode;
}

Segment * ModelView::getCurrentSegment()
{
  return m_currSegment;
}

int ModelView::getEpisodeIndex(Episode *episode)
{
  return m_episodes.indexOf(episode);
}

/////////////////////
// private methods //
/////////////////////

void ModelView::updateCurrentEpisode(Segment *segment)
{
  Episode *episode = 0;

  // looking for corresponding episode
  Segment *tmp = segment;

  // searching among parent segments
  while (tmp != 0 && !(episode = dynamic_cast<Episode *>(tmp)))
    tmp = tmp->parent();

  // if not found, searching among child segments
  if (!tmp) {
    tmp = segment;
    while (tmp != 0 && !(episode = dynamic_cast<Episode *>(tmp)))
      tmp = tmp->child(0);
  }

  if (episode != m_currEpisode) {
    m_currEpisode = episode;
    emit setPlayer(m_currEpisode->getFName(), m_episodes.indexOf(m_currEpisode), m_currEpisode->getResolution());
    emit setEpisode(m_currEpisode);
  }
}

QList<qint64> ModelView::genPositionList(Segment *segment)
{
  QList<qint64> positionList;
  qint64 segmentPosition;
  Segment *currParent;
  Segment *prevParent;
  Segment *nextParent;

  int iCurrParent;
  int iPrevParent;
  int iNextParent;

  int iCurr = segment->row();
  int iPrev(iCurr);
  int iNext(iCurr);
  int nPrev = m_nVignettes / 2;
  int nNext(nPrev);
  
  // adding current position
  currParent = segment->parent();
  segmentPosition = currParent->child(iCurr)->getPosition();
  positionList.push_back(segmentPosition);

  // parent index
  iCurrParent = currParent->row();
  
  // adding previous positions
  prevParent = currParent;
  iPrevParent = iCurrParent;
  while (nPrev > 0) {
    iPrev--;

    if (iPrev >= 0) {
      segmentPosition = prevParent->child(iPrev)->getPosition();
      positionList.push_front(segmentPosition);
    }

    else {
      iPrevParent--;

      if (iPrevParent >= 0) {
	prevParent = prevParent->parent()->child(iPrevParent);
	iPrev = prevParent->childCount() - 1;
	segmentPosition = prevParent->child(iPrev)->getPosition();
	positionList.push_front(segmentPosition);
      }

      else
	positionList.push_front(-1);
    }

    nPrev--;
  }

  // adding next positions
  nextParent = currParent;
  iNextParent = iCurrParent;
  while (nNext > 0) {
    iNext++;

    if (iNext < nextParent->childCount()) {
      segmentPosition = nextParent->child(iNext)->getPosition();
      positionList.push_back(segmentPosition);
    }
    
    else {
      iNextParent++;

      if (iNextParent < nextParent->parent()->childCount()) {
	nextParent = nextParent->parent()->child(iNextParent);
	iNext = 0;
	segmentPosition = nextParent->child(iNext)->getPosition();
	positionList.push_back(segmentPosition);
      }

      else
	positionList.push_back(-1);
    }

    nNext--;
  }

  return positionList;
}
