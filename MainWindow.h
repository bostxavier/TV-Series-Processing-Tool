#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QModelIndex>

#include <armadillo>

#include "Segment.h"
#include "VideoPlayer.h"
#include "ProjectModel.h"
#include "ModelView.h"

class MainWindow: public QMainWindow
{
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = 0);
  ~MainWindow();

  public slots:
    bool newProject();
    bool openProject();
    bool saveProject();
    bool closeProject();
    bool quitTool();
    bool addNewEpisode();
    bool addProject();
    bool addSubtitles();
    bool showHisto();
    bool extractShots();
    bool detectSimShots();
    bool extractScenes();
    bool spkDiar();
    bool spkInteract();
    bool musicTracking();
    bool coClustering();
    void processReturn(Segment *segment);
    void processDel(Segment *segment);
    void shotSimAnnotation(bool checked);
    void selectShotLevel();
    void selectSceneLevel();
    void selectEpisodeLevel();
    void selectSeasonLevel();
    void shotAnnotation(bool checked);
    void sceneAnnotation(bool checked);
    void modelChanged();
    void viewSegmentation();
    void viewSpeakers();
    void updateSpeakers();
    void viewLocSpkDiar();
    void viewFaceBound(bool checked);
    bool faceDetection();
    void exportShotBound();
    bool summarization();
    void playLsus(QList<QPair<Episode *, QPair<qint64, qint64> > > segments);
    bool viewNarrChart();

 signals:
    void activeHisto(bool histoDisp);
    void insertSegment(Segment *segment, Segment::Source source);
    void removeSegment(Segment *segment);
    void showVignette(bool showed);
    void processSegmentation(bool autoChecked, bool refChecked, bool annot);
    void extractShotBound(const QString &fName);
    void viewUtteranceTree(bool checked);
    void extractIVectors(bool ubm, const QString &epFName, const QString &epNumber);
    void extractSpkIVectors(const QString &epFName, const QString &epNumber, bool refSpk);
    void extractUttShotVectors();
    void setSubReadOnly(bool readOnly);
    void showNarrChart(const QMap<QString, QList<QPoint> > &narrChart);

 private:
  void createActions();
  void createMenus();
  void updateActions();
  void selectDefaultLevel();
  
  QAction *m_newProAct;
  QAction *m_proOpenProAct;
  QAction *m_saveProAct;
  QAction *m_closeProAct;
  QAction *m_quitAct;
  QAction *m_addProAct;
  QAction *m_addNewEpAct;
  QAction *m_addSubAct;
  QAction *m_histoAct;
  QAction *m_viewAutoSegAct;
  QAction *m_viewRefSegAct;
  QAction *m_viewSpeakersAct;
  QAction *m_viewLocSpkDiarAct;
  QAction *m_viewFaceAct;
  QAction *m_shotLevelAct;
  QAction *m_sceneLevelAct;
  QAction *m_episodeLevelAct;
  QAction *m_seasonLevelAct;
  QAction *m_manShotAct;
  QAction *m_coClusterAct;
  QAction *m_autShotAct;
  QAction *m_spkDiarAct;
  QAction *m_spkInteractAct;
  QAction *m_musicTrackAct;
  QAction *m_manShotSimAct;
  QAction *m_simShotDetectAct;
  QAction *m_speechSegAct;
  QAction *m_exportShotBoundAct;
  QAction *m_manSceneAct;
  QAction *m_autSceneAct;
  QAction *m_faceDetectAct;
  QAction *m_summAct;
  QAction *m_chartAct;

  QActionGroup *m_granularityGroup;
 
  QMenu *m_viewSegMenu;
  QMenu *m_annotationsMenu;

  bool m_proOpen;
  bool m_proModified;
  bool m_histoDisp;

  VideoPlayer *m_videoPlayer;
  ProjectModel *m_project;
  ModelView *m_modelView;
};

#endif
