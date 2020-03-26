#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <QWidget>
#include <QTreeView>
#include <QItemSelectionModel>
#include <QKeyEvent>

#include "ProjectModel.h"
#include "Segment.h"
#include "Episode.h"

class ModelView: public QWidget
{
  Q_OBJECT

 public:
  ModelView(ProjectModel *project, int nVignettes = 5, QWidget *parent = 0);
  void adjust();
  void initPlayer();
  QString getCurrentEpisodeFName() const;
  QString getCurrentEpisodeNumber() const;
  QSize getCurrentEpisodeRes() const;
  Episode * getCurrentEpisode();
  Segment * getCurrentSegment();
  int getEpisodeIndex(Episode *episode);

  public slots:
    void initEpisodes(QList<Episode *> episodes);
    void insertEpisode(int index, Episode *episode);
    void updatePlayer(const QModelIndex &current, const QModelIndex &previous);
    void updateCurrentSegment(const QModelIndex &current, const QModelIndex &previous);
    void positionChanged(qint64 position);
    void episodeIndexChanged(int index);
    void playerPaused(bool pause);
    void updateEpisode(Episode *episode);
    void setDepth(int depth);

  signals:
    void setPlayer(const QString &fName, int index, const QSize &resolution);
    void setPlayerPosition(qint64);
    void setEpisode(Episode *episode);
    void initShotLevel(Segment *segment);
    void initSceneLevel(Segment *segment);
    void returnPressed(Segment *segment);
    void delPressed(Segment *segment);
    void currentShot(QList<qint64>);
    void newManPosition(qint64 position);
    void resetSpeakersView();

 protected:
    void keyPressEvent(QKeyEvent *event);
  
 private:
    void updateCurrentEpisode(Segment *segment);
    QList<qint64> genPositionList(Segment *segment);

    Segment *m_currSegment;
    Episode *m_currEpisode;
    QList<Episode *> m_episodes;
    ProjectModel *m_project;
    QTreeView *m_treeView;
    QItemSelectionModel *m_selection;
    int m_depth;
    int m_nVignettes;
    bool m_playerPaused;
};

#endif
