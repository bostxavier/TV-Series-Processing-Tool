#include <QStyle>
#include <QMenuBar>
#include <QApplication>
#include <QHBoxLayout>
#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QVideoFrame>

#include "MainWindow.h"
#include "NewProjectDialog.h"
#include "AddProjectDialog.h"
#include "CompareImageDialog.h"
#include "MusicTrackingDialog.h"
#include "FaceDetectDialog.h"
#include "GetFileDialog.h"
#include "Scene.h"
#include "Shot.h"
#include "SpeechSegment.h"
#include "VideoFrame.h"
#include "EditSimShotDialog.h"
#include "SceneExtractDialog.h"
#include "SummarizationDialog.h"
#include "Face.h"

using namespace arma;

/////////////////
// constructor //
/////////////////

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent)
{
  setWindowTitle("TV Series Processing Tool");
  createActions();
  createMenus();

  // connecting action signals to slots
  connect(m_newProAct, SIGNAL(triggered()), this, SLOT(newProject()));
  connect(m_proOpenProAct, SIGNAL(triggered()), this, SLOT(openProject()));
  connect(m_saveProAct, SIGNAL(triggered()), this, SLOT(saveProject()));
  connect(m_closeProAct, SIGNAL(triggered()), this, SLOT(closeProject()));
  connect(m_addNewEpAct, SIGNAL(triggered()), this, SLOT(addNewEpisode()));
  connect(m_addProAct, SIGNAL(triggered()), this, SLOT(addProject()));
  connect(m_addSubAct, SIGNAL(triggered()), this, SLOT(addSubtitles()));
  connect(m_quitAct, SIGNAL(triggered()), this, SLOT(quitTool()));
  connect(m_histoAct, SIGNAL(triggered()), this, SLOT(showHisto()));
  connect(m_autShotAct, SIGNAL(triggered()), this, SLOT(extractShots()));
  connect(m_simShotDetectAct, SIGNAL(triggered()), this, SLOT(detectSimShots()));
  connect(m_spkDiarAct, SIGNAL(triggered()), this, SLOT(spkDiar()));
  connect(m_spkInteractAct, SIGNAL(triggered()), this, SLOT(spkInteract()));
  connect(m_musicTrackAct, SIGNAL(triggered()), this, SLOT(musicTracking()));
  connect(m_shotLevelAct, SIGNAL(triggered()), this, SLOT(selectShotLevel()));
  connect(m_sceneLevelAct, SIGNAL(triggered()), this, SLOT(selectSceneLevel()));
  connect(m_episodeLevelAct, SIGNAL(triggered()), this, SLOT(selectEpisodeLevel()));
  connect(m_seasonLevelAct, SIGNAL(triggered()), this, SLOT(selectSeasonLevel()));
  connect(m_manShotAct, SIGNAL(triggered(bool)), this, SLOT(shotAnnotation(bool)));
  connect(m_manShotSimAct, SIGNAL(triggered(bool)), this, SLOT(shotSimAnnotation(bool)));
  connect(m_faceDetectAct, SIGNAL(triggered()), this, SLOT(faceDetection()));
  connect(m_exportShotBoundAct, SIGNAL(triggered()), this, SLOT(exportShotBound()));
  connect(m_viewLocSpkDiarAct, SIGNAL(triggered()), this, SLOT(viewLocSpkDiar()));
  connect(m_viewFaceAct, SIGNAL(triggered(bool)), this, SLOT(viewFaceBound(bool)));
  connect(m_manSceneAct, SIGNAL(triggered(bool)), this, SLOT(sceneAnnotation(bool)));
  connect(m_autSceneAct, SIGNAL(triggered()), this, SLOT(extractScenes()));
  connect(m_coClusterAct, SIGNAL(triggered()), this, SLOT(coClustering()));
  connect(m_summAct, SIGNAL(triggered()), this, SLOT(summarization()));
  connect(m_chartAct, SIGNAL(triggered()), this, SLOT(viewNarrChart()));

  // enabling/disabling actions
  m_proOpen = false;
  m_proModified = false;
  m_histoDisp = false;
  updateActions();

  // initializing model/view
  m_project = new ProjectModel(this);
  m_modelView = new ModelView(m_project);

  // initializing video player
  m_videoPlayer = new VideoPlayer;
  m_videoPlayer->showVignette(false);

  // initializing central area
  QHBoxLayout *viewLayout = new QHBoxLayout;
  viewLayout->addWidget(m_videoPlayer);
  viewLayout->addWidget(m_modelView);
  QWidget *centralWidget = new QWidget;
  centralWidget->setLayout(viewLayout);
  setCentralWidget(centralWidget);

  // connecting model view signals to player slots
  connect(m_modelView, SIGNAL(setPlayer(const QString &, int, const QSize &)), m_videoPlayer, SLOT(setPlayer(const QString &, int, const QSize &)));
  connect(m_modelView, SIGNAL(setPlayerPosition(qint64)), m_videoPlayer, SLOT(setPlayerPosition(qint64)));
  connect(m_modelView, SIGNAL(setEpisode(Episode *)), m_project, SLOT(setEpisode(Episode *)));
  connect(m_modelView, SIGNAL(currentShot(QList<qint64>)), m_videoPlayer, SLOT(currentShot(QList<qint64>)));
  connect(this, SIGNAL(showVignette(bool)), m_videoPlayer, SLOT(showVignette(bool)));

  connect(m_viewSpeakersAct, SIGNAL(triggered()), this, SLOT(viewSpeakers()));
  connect(m_viewSpeakersAct, SIGNAL(triggered(bool)), m_videoPlayer, SLOT(viewSpeakers(bool)));

  // possibly connecting model view signal to model slot
  connect(m_modelView, SIGNAL(returnPressed(Segment *)), this, SLOT(processReturn(Segment *)));
  connect(m_modelView, SIGNAL(delPressed(Segment *)), this, SLOT(processDel(Segment *)));
  connect(this, SIGNAL(insertSegment(Segment *, Segment::Source)), m_project, SLOT(insertSegment(Segment *, Segment::Source)));
  connect(this, SIGNAL(removeSegment(Segment *)), m_project, SLOT(removeSegment(Segment *)));
  connect(m_modelView, SIGNAL(newManPosition(qint64)), m_project, SLOT(setCurrSpeechSeg(qint64)));
  connect(m_modelView, SIGNAL(newManPosition(qint64)), m_project, SLOT(getCurrLSU(qint64)));

  // connecting player signal to model view slot
  connect(m_videoPlayer, SIGNAL(positionUpdated(qint64)), m_modelView, SLOT(positionChanged(qint64)));
  connect(m_videoPlayer, SIGNAL(playerPaused(bool)), m_modelView, SLOT(playerPaused(bool)));
  connect(m_videoPlayer, SIGNAL(episodeIndexChanged(int)), m_modelView, SLOT(episodeIndexChanged(int)));
  
  // connecting player signal to model slots
  connect(m_videoPlayer, SIGNAL(positionUpdated(qint64)), m_project, SLOT(setCurrSpeechSeg(qint64)));
  connect(m_videoPlayer, SIGNAL(positionUpdated(qint64)), m_project, SLOT(getCurrLSU(qint64)));

  // connecting view actions to monitoring widget
  connect(this, SIGNAL(activeHisto(bool)), m_videoPlayer, SLOT(activeHisto(bool)));
  connect(this, SIGNAL(viewUtteranceTree(bool)), m_videoPlayer, SLOT(viewUtteranceTree(bool)));
  connect(m_project, SIGNAL(getCurrentPattern(const QPair<int, int> &)), m_videoPlayer, SLOT(getCurrentPattern(const QPair<int, int> &)));
  connect(m_project, SIGNAL(initDiarData(const arma::mat &, const arma::mat &, const arma::mat &, QList<SpeechSegment *>)), m_videoPlayer, SLOT(setDiarData(const arma::mat &, const arma::mat &, const arma::mat &, QList<SpeechSegment *>)));

  // indirectly connecting annotation tools actions to model slots
  connect(m_manShotAct, SIGNAL(triggered(bool)), m_project, SLOT(initShotAnnot(bool)));

  // connecting model view signals to segments view slots
  connect(m_project, SIGNAL(resetSegmentView()), this, SLOT(viewSegmentation()));
  connect(m_modelView, SIGNAL(resetSpeakersView()), this, SLOT(updateSpeakers()));
  connect(m_project, SIGNAL(resetSpkDiarMonitor()), this, SLOT(viewLocSpkDiar()));

  // emitting appropriate signal when segmentation view/annotation action is triggered
  connect(m_viewAutoSegAct, SIGNAL(triggered()), this, SLOT(viewSegmentation()));
  connect(m_viewRefSegAct, SIGNAL(triggered()), this, SLOT(viewSegmentation()));
  connect(m_speechSegAct, SIGNAL(triggered()), this, SLOT(viewSegmentation()));
  connect(this, SIGNAL(processSegmentation(bool, bool, bool)), m_project, SLOT(processSegmentation(bool, bool, bool)));
  
  // spoken frames coming from model sent to player for monitoring purpose; end signal
  connect(m_project, SIGNAL(getShots(QList<Segment *>, Segment::Source)), m_videoPlayer, SLOT(getShots(QList<Segment *>, Segment::Source)));
  connect(m_project, SIGNAL(getSpeechSegments(QList<Segment *>, Segment::Source)), m_videoPlayer, SLOT(getSpeechSegments(QList<Segment *>, Segment::Source)));
  connect(m_project, SIGNAL(getRefSpeakers(const QStringList &)), m_videoPlayer, SLOT(getRefSpeakers(const QStringList &)));
  connect(m_project, SIGNAL(viewSegmentation(bool, bool)), m_videoPlayer, SLOT(showSegmentation(bool, bool)));

  // connecting model signals to view and player slots
  connect(m_project, SIGNAL(setEpisodes(QList<Episode *>)), m_modelView, SLOT(initEpisodes(QList<Episode *>)));
  connect(m_project, SIGNAL(insertEpisode(int, Episode *)), m_modelView, SLOT(insertEpisode(int, Episode *)));
  connect(m_project, SIGNAL(positionChanged(qint64)), m_modelView, SLOT(positionChanged(qint64)));
  connect(m_project, SIGNAL(setDepthView(int)), m_modelView, SLOT(setDepth(int)));
  connect(m_project, SIGNAL(updateEpisode(Episode *)), m_modelView, SLOT(updateEpisode(Episode *)));
  connect(m_project, SIGNAL(addMedia(const QString &)), m_videoPlayer, SLOT(addMedia(const QString &)));
  connect(m_project, SIGNAL(insertMedia(int, const QString &)), m_videoPlayer, SLOT(insertMedia(int, const QString &)));
  connect(m_project, SIGNAL(viewSpkNet(QList<QList<SpeechSegment *> >)), m_videoPlayer, SLOT(viewSpkNet(QList<QList<SpeechSegment *> >)));
  connect(m_project, SIGNAL(updateSpkView(int)), m_videoPlayer, SLOT(updateSpkView(int)));
  connect(m_project, SIGNAL(displaySubtitle(SpeechSegment *)), m_videoPlayer, SLOT(displaySubtitle(SpeechSegment *)));
  connect(m_project, SIGNAL(playLsus(QList<QPair<Episode *, QPair<qint64, qint64> > >)), this, SLOT(playLsus(QList<QPair<Episode *, QPair<qint64, qint64> > >)));
  connect(m_project, SIGNAL(grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)), m_videoPlayer, SLOT(grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &)));

  // miscellaneous
  connect(this, SIGNAL(setSubReadOnly(bool)), m_videoPlayer, SLOT(setSubReadOnly(bool)));

  connect(m_project, SIGNAL(viewNarrChart(QList<QList<SpeechSegment *> >)), m_videoPlayer, SLOT(viewNarrChart(QList<QList<SpeechSegment *> >)));
}

////////////////
// destructor //
////////////////

MainWindow::~MainWindow()
{
}

//////////////////////////////////////////
// slots called when triggering actions //
//////////////////////////////////////////

bool MainWindow::newProject()
{
  NewProjectDialog dialog(this);

  if (dialog.exec() == QDialog::Accepted) {

    //////////////////////////////
    // initializing new project //
    //////////////////////////////

    m_project->setModel(dialog.getProjectName(),
			dialog.getSeriesName(),
			dialog.getSeasNbr(),
			dialog.getEpNbr(),
			dialog.getEpName(),
			dialog.getEpFName());
    setWindowTitle("TV Series Processing Tool - " + m_project->getName());
    m_modelView->initPlayer();
    m_proOpen = true;
    m_proModified = true;
    updateActions();

    return true;
  }

  return false;
}
 
bool MainWindow::openProject()
{
  QString fName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(), tr("Project (*.json)"));
  // QString fName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(), tr("Project (*.dat)"));

  if (!fName.isEmpty() && m_project->load(fName)) {

    m_project->initEpisodes();
    m_modelView->initPlayer();
    setWindowTitle("TV Series Processing Tool - " + m_project->getName());
    selectDefaultLevel();

    m_proOpen = true;
    updateActions();
    
    return true;
  }

  return false;
}

bool MainWindow::saveProject()
{
  // desactivate shot insertion if checked
  // otherwise, video frames would be saved
  if (m_manShotAct->isChecked()) {
    m_manShotAct->setChecked(false);
    m_project->initShotAnnot(false);
  }

  QString fName = QFileDialog::getSaveFileName(this, tr("Save File"),
					       ("./projects/" + m_project->getName()),
					       tr("Project (*.json)"));

  // QString fName = QFileDialog::getSaveFileName(this, tr("Save File"),
  // ("./projects/" + m_project->getName()),
  // tr("Project (*.dat)"));

  if (m_project->save(fName)) {
    m_proModified = false;
    updateActions();
    return true;
  }

  return false;
}
 
bool MainWindow::closeProject()
{
  int ans = 0;

  if (m_proModified)
    ans = QMessageBox::question(this, tr("Close project"), tr("Current project has been modified, save changes ?"), QMessageBox::Yes | QMessageBox::No);

  if (ans == QMessageBox::Yes)
    saveProject();

  setWindowTitle("TV Series Processing Tool");
  // m_project->reset();
  m_videoPlayer->reset();

  m_proModified = false;
  m_proOpen = false;
  updateActions();

  return true;
}

bool MainWindow::addNewEpisode()
{
  NewProjectDialog dialog(this, m_project->getName(), m_project->getSeriesName());

  if (dialog.exec() == QDialog::Accepted) {

    selectShotLevel();

    if (m_project->addNewEpisode(dialog.getSeasNbr(), dialog.getEpNbr(), dialog.getEpName(), dialog.getEpFName())) {
      
      selectDefaultLevel();
      m_proModified = true;
      updateActions();

      return true;
    }

    else {
      QString errMsg = "Impossible to add episode ";
      errMsg += QString::number(dialog.getEpNbr());
      errMsg += " of season ";
      errMsg += QString::number(dialog.getSeasNbr());
      errMsg += ": episode already included in project.";
      QMessageBox::critical(this, tr("Add new project"), errMsg);
    }
  }

  return false;
}

bool MainWindow::addProject()
{
  AddProjectDialog dialog(m_project->getName(), m_project->getSeriesName(), this);

  if (dialog.exec() == QDialog::Accepted) {
    
    selectShotLevel();

    if (m_project->addModel(dialog.getProjectName(), dialog.getSecProjectFName())) {

      selectDefaultLevel();
      m_proModified = true;
      updateActions();

      return true;
    }

    else {
      QString errMsg = "Impossible to add a project related to another serie";
      QMessageBox::critical(this, tr("Add new project"), errMsg);
    }
  }

  return false;
}

bool MainWindow::addSubtitles()
{
  GetFileDialog dialog(tr("Subtitles File"), tr("Subtitles Files (*.json)"), this);

  if (dialog.exec() == QDialog::Accepted) {

    m_project->insertSubtitles(dialog.getFName());

    m_proModified = true;
    updateActions();

    return true;
  }

  return false;
}

bool MainWindow::quitTool()
{
  int ans = 0;

  if (m_proModified)
    ans = QMessageBox::question(this, tr("Quit"), tr("Current project has been modified, save changes?"), QMessageBox::Yes | QMessageBox::No);

  if (ans == QMessageBox::Yes)
    saveProject();

  QCoreApplication::quit();
  
  return true;
}

bool MainWindow::showHisto()
{
  if (m_proOpen && !m_histoDisp) {
    m_histoDisp = true;
  }
  else
    m_histoDisp = false;

  emit activeHisto(m_histoDisp);
  
  updateActions();
  return true;
}

bool MainWindow::extractShots()
{
  CompareImageDialog dialog(tr("Shot extraction"), 64, 24, 8, 30, 20, tr("<b>High threshold:</b>"), tr("<b>Low threshold:</b>"), this);

  if (dialog.exec() == QDialog::Accepted) {

    // selectVideoFrameLevel();
    m_project->extractShots(m_modelView->getCurrentEpisodeFName(),
			    dialog.getHistoType(),
			    dialog.getNVBins(),
			    dialog.getNHBins(),
			    dialog.getNSBins(),
			    dialog.getMetrics(),
			    dialog.getThreshold1(),
			    dialog.getThreshold2(),
			    dialog.getNVBlock(),
			    dialog.getNHBlock());
    selectShotLevel();
    m_proModified = true;
    updateActions();

    return true;
  }

  return false;
}

bool MainWindow::detectSimShots()
{
  CompareImageDialog dialog(tr("Shot Similarity Detection..."), 64, 24, 8, 40, 30, tr("<b>Max distance:</b>"), tr("<b>Window size:</b>"), this);

  if (dialog.exec() == QDialog::Accepted) {
    
    selectShotLevel();
    m_project->labelSimilarShots(m_modelView->getCurrentEpisodeFName(),
				 dialog.getHistoType(),
				 dialog.getNVBins(),
				 dialog.getNHBins(),
				 dialog.getNSBins(),
				 dialog.getMetrics(),
				 dialog.getThreshold1(),
				 dialog.getThreshold2(),
				 dialog.getNVBlock(),
				 dialog.getNHBlock());
    selectShotLevel();
    m_proModified = true;
    updateActions();
 
    return true;
  }

  return false;
}

bool MainWindow::extractScenes()
{
  SceneExtractDialog dialog(this);

  if (dialog.exec() == QDialog::Accepted) {
    selectShotLevel();
    m_project->extractScenes(dialog.getVisualSource(),
			     m_modelView->getCurrentEpisodeFName());
    selectShotLevel();
    updateActions();

    return true;
  }

  return false;
}

bool MainWindow::spkDiar()
{
  SpkDiarizationDialog::Method method;
  SpkDiarizationDialog dialog(tr("Speaker diarization"), false, false, this);

  if (dialog.exec() == QDialog::Accepted) {

    method = dialog.getMethod();

    if (method != SpkDiarizationDialog::Ref) {

      m_project->localSpkDiar(dialog.getMethod(),
			      dialog.getDist(),
			      dialog.getNorm(),
			      dialog.getAgrCrit(),
			      dialog.getPartMeth(),
			      dialog.getWeight(),
			      dialog.getSigma());
    }
    
    // m_project->globalSpkDiar();
    
    m_proModified = true;
    updateActions();

    return true;
  }

  return false;
}

bool MainWindow::spkInteract()
{
  SpkInteractDialog dialog(tr("Speakers interactions"), this);

  if (dialog.exec() == QDialog::Accepted) {
    
    m_project->spkInteract(dialog.getInteractType(),
			   dialog.getRefUnit(),
			   dialog.getNbDiscard(),
			   dialog.getInterThresh());

    m_proModified = true;
    updateActions();

    return true;
  }

  return false;
}

bool MainWindow::musicTracking()
{
  MusicTrackingDialog dialog(tr("Music tracking"), this);

  if (dialog.exec() == QDialog::Accepted) {
    m_project->musicTracking(dialog.getSampleRate(),
			     dialog.getMtWindowSize(),
			     dialog.getMtHopSize(),
			     dialog.getChromaStaticFrameSize(),
			     dialog.getChromaDynamicFrameSize());

    m_proModified = true;
    updateActions();

    return true;
  }

  return false;
}

bool MainWindow::coClustering()
{
  m_project->coClustering(m_modelView->getCurrentEpisodeFName());

  m_proModified = true;
  updateActions();

  return true;
}

void MainWindow::selectShotLevel()
{
  m_shotLevelAct->setChecked(true);
  m_modelView->setDepth(m_project->getDepth());
  emit showVignette(true);
}

void MainWindow::selectSceneLevel()
{
  m_sceneLevelAct->setChecked(true);
  m_modelView->setDepth(m_project->getDepth() - 1);
  emit showVignette(false);
}

void MainWindow::selectEpisodeLevel()
{
  m_episodeLevelAct->setChecked(true);
  m_modelView->setDepth(m_project->getDepth() - 2);
  emit showVignette(false);
}

void MainWindow::selectSeasonLevel()
{
  m_seasonLevelAct->setChecked(true);
  m_modelView->setDepth(m_project->getDepth() - 3);
  emit showVignette(false);
}

void MainWindow::processReturn(Segment *segment)
{
  if (m_manShotAct->isChecked() && dynamic_cast<VideoFrame *>(segment))
    emit insertSegment(segment, Segment::Manual);

  else if (m_manSceneAct->isChecked() && dynamic_cast<Shot *>(segment))
    emit insertSegment(segment, Segment::Manual);

  else if (m_manShotSimAct->isChecked() && dynamic_cast<Shot *>(segment)) {

    QList<Shot *> shots;
    m_project->retrieveShots(m_modelView->getCurrentEpisode(), shots);
    EditSimShotDialog annotSimDialog(m_modelView->getCurrentEpisodeFName(),
				     0,
				     shots,
				     6,
				     1000,
				     this);

    annotSimDialog.exec();
    m_proModified = true;
    updateActions();
    
  }
}

void MainWindow::processDel(Segment *segment)
{
  if (m_manShotAct->isChecked() && dynamic_cast<Shot *>(segment))
    emit removeSegment(segment);

  else if (m_manSceneAct->isChecked() && dynamic_cast<Scene *>(segment))
    emit removeSegment(segment);
}

void MainWindow::shotAnnotation(bool checked)
{
  if (checked) {
    
    selectShotLevel();
    m_shotLevelAct->setEnabled(false);
    m_sceneLevelAct->setEnabled(false);
    m_episodeLevelAct->setEnabled(false);
    m_seasonLevelAct->setEnabled(false);
    m_proModified = true;
    updateActions();
  }
  else {
    selectDefaultLevel();
    updateActions();
  }
}

void MainWindow::shotSimAnnotation(bool checked)
{
  if (checked) {
    selectShotLevel();
    m_sceneLevelAct->setEnabled(false);
    m_episodeLevelAct->setEnabled(false);
    m_seasonLevelAct->setEnabled(false);
  }
  else {
    selectDefaultLevel();
    updateActions();
  }
}

void MainWindow::sceneAnnotation(bool checked)
{
  if (checked) {
    selectShotLevel();
    m_sceneLevelAct->setEnabled(false);
    m_proModified = true;
    updateActions();
  }
  else {
    selectDefaultLevel();
    updateActions();
  }
}

void MainWindow::modelChanged()
{
  m_proModified = true;
  selectDefaultLevel();
  updateActions();
}

void MainWindow::viewSegmentation()
{
  emit processSegmentation(m_viewAutoSegAct->isChecked(), m_viewRefSegAct->isChecked(), m_speechSegAct->isChecked());

  if (m_speechSegAct->isChecked()) {

    m_proModified = true;
    updateActions();
  }

  emit setSubReadOnly(!m_speechSegAct->isChecked());
}

void MainWindow::viewSpeakers()
{
  Segment *segment = m_modelView->getCurrentSegment();

  if (m_viewSpeakersAct->isChecked())
    m_project->retrieveSpeakersNet(segment);
}

void MainWindow::updateSpeakers()
{
  Segment *segment = m_modelView->getCurrentSegment();

  if (m_viewSpeakersAct->isChecked())
    m_project->updateSpeakersNet(segment);
}

void MainWindow::viewLocSpkDiar()
{
  if (m_viewLocSpkDiarAct->isChecked())
    m_project->getDiarData();
   
  emit viewUtteranceTree(m_viewLocSpkDiarAct->isChecked());

  updateActions();
}

void MainWindow::viewFaceBound(bool checked)
{
  if (checked) {
    connect(m_videoPlayer, SIGNAL(positionUpdated(qint64)), m_project, SLOT(setCurrShot(qint64)));
    connect(m_videoPlayer, SIGNAL(positionUpdated(qint64)), m_project, SLOT(getCurrFaces(qint64)));
    connect(m_modelView, SIGNAL(newManPosition(qint64)), m_project, SLOT(setCurrShot(qint64)));
    connect(m_modelView, SIGNAL(newManPosition(qint64)), m_project, SLOT(getCurrFaces(qint64)));
    // connect(m_project, SIGNAL(displayFaceBound(const QList<QRect> &)), m_videoPlayer, SLOT(displayFaceBound(const QList<QRect> &)));
    // connect(m_project, SIGNAL(displayFaceLandmarks(const QList<QPoint> &)), m_videoPlayer, SLOT(displayFaceLand(const QList<QPoint> &)));
    connect(m_project, SIGNAL(displayFaces(const QList<Face> &)), m_videoPlayer, SLOT(displayFaceList(const QList<Face> &)));
  }
  else {
    disconnect(m_videoPlayer, SIGNAL(positionUpdated(qint64)), m_project, SLOT(setCurrShot(qint64)));
    disconnect(m_videoPlayer, SIGNAL(positionUpdated(qint64)), m_project, SLOT(getCurrFaces(qint64)));
    disconnect(m_modelView, SIGNAL(newManPosition(qint64)), m_project, SLOT(setCurrShot(qint64)));
    disconnect(m_modelView, SIGNAL(newManPosition(qint64)), m_project, SLOT(getCurrFaces(qint64)));
    // disconnect(m_project, SIGNAL(displayFaceBound(const QList<QRect> &)), m_videoPlayer, SLOT(displayFaceBound(const QList<QRect> &)));
    // disconnect(m_project, SIGNAL(displayFaceLandmarks(const QList<QPoint> &)), m_videoPlayer, SLOT(displayFaceLand(const QList<QPoint> &)));
    disconnect(m_project, SIGNAL(displayFaces(const QList<Face> &)), m_videoPlayer, SLOT(displayFaceList(const QList<Face> &)));
  }

  updateActions();
}

bool MainWindow::faceDetection()
{
  FaceDetectDialog dialog(tr("Face Detection"), this);

  if (dialog.exec() == QDialog::Accepted) {

    switch (dialog.getMethod()) {

    case FaceDetectDialog::OpenCV:
    case FaceDetectDialog::Zhu:
      m_project->faceDetection(m_modelView->getCurrentEpisodeFName(),
			       dialog.getMethod(),
			       dialog.getMinHeight());
      break;

    case FaceDetectDialog::ExtData:
      QString fName = QFileDialog::getOpenFileName(this, tr("Open Face Detection File"), "/home/xavier/programmes/face_detection/output/bound", tr("CSV (*.csv)"));

      if (!fName.isEmpty())
	m_project->externFaceDetection(fName);
      break;
    }
    
    m_proModified = true;
    updateActions();

    return true;
  }

  return false;
}

void MainWindow::exportShotBound()
{
  QString fName = QFileDialog::getSaveFileName(this, tr("Export shot boundaries"), tr("/home/xavier/programmes/face_detection/lst"), tr("CSV file (*.csv)"));
  
  if (!fName.isEmpty())
    m_project->exportShotBound(fName);
}

bool MainWindow::summarization()
{
  QStringList seasons = m_project->getSeasons();
  QStringList allSpeakers = m_project->retrieveRefSpeakers();
  QList<QStringList> seasonSpeakers = m_project->getSeasonSpeakers();

  SummarizationDialog dialog(tr("Summarization"), seasons, allSpeakers, seasonSpeakers, this);

  if (dialog.exec() == QDialog::Accepted) {
    
    return m_project->summarization(dialog.getMethod(),
				    dialog.getSeasonNb(),
				    dialog.getSpeaker(),
				    dialog.getDur(),
				    dialog.getGranu());
  }

  return false;
}

bool MainWindow::viewNarrChart()
{
  m_project->showNarrChart(m_chartAct->isChecked());

  return true;
}

void MainWindow::playLsus(QList<QPair<Episode *, QPair<qint64, qint64> > > segments)
{
  QMap<int, QList<QPair<qint64, qint64> > > playlist;

  for (int i(0); i < segments.size(); i++) {
    int episodeIndex = m_modelView->getEpisodeIndex(segments[i].first);
    playlist[episodeIndex].push_back(segments[i].second);
  }

  QList<QPair<int, QList<QPair<qint64, qint64> > > > summary;
  QMap<int, QList<QPair<qint64, qint64> > >::const_iterator it = playlist.begin();

  while (it != playlist.end()) {

    int index = it.key();
    QList<QPair<qint64, qint64> > bound = it.value();

    summary.push_back(QPair<int, QList<QPair<qint64, qint64> > >(index, bound));

    it++;
  }

  m_videoPlayer->setSummary(summary);
  m_videoPlayer->playSummary();
}

///////////////////////////////////
// auxiliary methods called when //
//   constructing main window    //
///////////////////////////////////

void MainWindow::createActions()
{
  // project menu actions
  m_newProAct = new QAction(tr("&New..."), this);
  m_proOpenProAct = new QAction(tr("&Open..."), this);
  m_saveProAct = new QAction(tr("&Save as..."), this);
  m_closeProAct = new QAction(tr("&Close"), this);
  m_quitAct = new QAction(tr("&Quit"), this);

  // edit menu actions
  m_addNewEpAct = new QAction(tr("Add &new episode..."), this);
  m_addProAct = new QAction(tr("&Add project..."), this);
  m_addSubAct = new QAction(tr("Add &subtitles..."), this);

  // view menu actions
  m_histoAct = new QAction(tr("&HSV histograms"), this);
  m_histoAct->setCheckable(true);
  m_viewAutoSegAct = new QAction(tr("Automatic"), this);
  m_viewAutoSegAct->setCheckable(true);
  m_viewRefSegAct = new QAction(tr("Reference"), this);
  m_viewRefSegAct->setCheckable(true);
  m_viewSpeakersAct = new QAction(tr("S&peakers network"), this);
  m_viewSpeakersAct->setCheckable(true);
  m_viewLocSpkDiarAct = new QAction(tr("&Local speaker diarization"), this);
  m_viewLocSpkDiarAct->setCheckable(true);
  m_viewFaceAct = new QAction(tr("&Face boundaries"), this);
  m_viewFaceAct->setCheckable(true);
  m_shotLevelAct = new QAction(tr("&Shot level"), this);
  m_sceneLevelAct = new QAction(tr("S&cene level"), this);
  m_episodeLevelAct = new QAction(tr("&Episode level"), this);
  m_seasonLevelAct = new QAction(tr("Se&ason level"), this);
  m_shotLevelAct->setCheckable(true);
  m_sceneLevelAct->setCheckable(true);
  m_episodeLevelAct->setCheckable(true);
  m_seasonLevelAct->setCheckable(true);
  m_chartAct = new QAction(tr("Narrative chart"), this);
  m_chartAct->setCheckable(true);

  // tools menu actions
  m_manShotAct = new QAction(tr("&Shot Insertion"), this);
  m_manShotSimAct = new QAction(tr("Shot Si&milarity"), this);
  m_manSceneAct = new QAction(tr("Sce&ne Insertion"), this);
  m_manShotAct->setCheckable(true);
  m_manShotSimAct->setCheckable(true);
  m_manSceneAct->setCheckable(true);
  m_speechSegAct = new QAction(tr("S&peech Segments"), this);
  m_speechSegAct->setCheckable(true);
  m_coClusterAct = new QAction(tr("Shot/Utterance co-&clustering"), this);
  m_autShotAct = new QAction(tr("Shot E&xtraction..."), this);
  m_simShotDetectAct = new QAction(tr("&Shot Similarity Detection..."), this);
  m_spkDiarAct = new QAction(tr("&Speaker Diarization..."), this);
  m_spkInteractAct = new QAction(tr("&Speakers interactions..."), this);
  m_musicTrackAct = new QAction(tr("Music &Tracking..."), this);
  m_autSceneAct = new QAction(tr("Scene E&xtraction..."), this);
  m_faceDetectAct = new QAction(tr("&Face Detection..."), this);
  m_exportShotBoundAct = new QAction(tr("Export shot boundaries..."), this);
  m_summAct = new QAction(tr("Su&mmarization..."), this);

  // set icon actions
  m_proOpenProAct->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
  m_saveProAct->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  m_closeProAct->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
}

void MainWindow::createMenus()
{
  // project menu
  QMenu *projectMenu = menuBar()->addMenu(tr("&Project"));
  projectMenu->addAction(m_newProAct);
  projectMenu->addAction(m_proOpenProAct);
  projectMenu->addSeparator();
  projectMenu->addAction(m_saveProAct);
  projectMenu->addAction(m_closeProAct);
  projectMenu->addSeparator();
  projectMenu->addAction(m_quitAct);
  
  // edit menu
  QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
  editMenu->addAction(m_addNewEpAct);
  editMenu->addAction(m_addProAct);
  editMenu->addAction(m_addSubAct);
  
  // view menu
  QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
  viewMenu->addAction(m_histoAct);
  m_viewSegMenu = viewMenu->addMenu(tr("&Segmentation"));
  m_viewSegMenu->addAction(m_viewAutoSegAct);
  m_viewSegMenu->addAction(m_viewRefSegAct);
  viewMenu->addAction(m_viewSpeakersAct);
  viewMenu->addAction(m_viewLocSpkDiarAct);
  viewMenu->addAction(m_viewFaceAct);
  viewMenu->addSeparator()->setText(tr("Depth"));
  m_granularityGroup = new QActionGroup(this);
  m_granularityGroup->addAction(m_shotLevelAct);
  m_granularityGroup->addAction(m_sceneLevelAct);
  m_granularityGroup->addAction(m_episodeLevelAct);
  m_granularityGroup->addAction(m_seasonLevelAct);
  viewMenu->addAction(m_shotLevelAct);
  viewMenu->addAction(m_sceneLevelAct);
  viewMenu->addAction(m_episodeLevelAct);
  viewMenu->addAction(m_seasonLevelAct);
  viewMenu->addSeparator();
  viewMenu->addAction(m_chartAct);

  // tools menu
  QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
  m_annotationsMenu = toolsMenu->addMenu(tr("&Annotations"));
  m_annotationsMenu->addAction(m_manShotAct);
  m_annotationsMenu->addAction(m_manShotSimAct);
  m_annotationsMenu->addAction(m_manSceneAct);
  m_annotationsMenu->addSeparator();
  m_annotationsMenu->addAction(m_speechSegAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(m_coClusterAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(m_spkDiarAct);
  toolsMenu->addAction(m_spkInteractAct);
  toolsMenu->addAction(m_musicTrackAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(m_autShotAct);
  toolsMenu->addAction(m_simShotDetectAct);
  toolsMenu->addAction(m_autSceneAct);
  toolsMenu->addAction(m_faceDetectAct);
  toolsMenu->addAction(m_exportShotBoundAct);
  toolsMenu->addSeparator();
  toolsMenu->addAction(m_summAct);
}

///////////////////////////////
//  auxiliary method called  //
// to enable/disable actions //
///////////////////////////////

void MainWindow::updateActions()
{
  m_newProAct->setEnabled(!m_proOpen);
  m_proOpenProAct->setEnabled(!m_proOpen);
  m_saveProAct->setEnabled(m_proModified);
  m_granularityGroup->setEnabled(m_proOpen);
  m_histoAct->setEnabled(m_proOpen);
  m_histoAct->setChecked(m_histoDisp);
  m_viewSegMenu->setEnabled(m_proOpen);
  m_viewSpeakersAct->setEnabled(m_proOpen);
  m_viewLocSpkDiarAct->setEnabled(m_proOpen);
  m_viewFaceAct->setEnabled(m_proOpen);
  m_closeProAct->setEnabled(m_proOpen);
  m_addNewEpAct->setEnabled(m_proOpen);
  m_addProAct->setEnabled(m_proOpen);
  m_addSubAct->setEnabled(m_proOpen);
  m_annotationsMenu->setEnabled(m_proOpen);
  m_autShotAct->setEnabled(m_proOpen);
  m_simShotDetectAct->setEnabled(m_proOpen);
  m_spkDiarAct->setEnabled(m_proOpen);
  m_spkInteractAct->setEnabled(m_proOpen);
  m_musicTrackAct->setEnabled(m_proOpen);
  m_faceDetectAct->setEnabled(m_proOpen);
  m_exportShotBoundAct->setEnabled(m_proOpen);
  m_autSceneAct->setEnabled(m_proOpen);
  m_coClusterAct->setEnabled(m_proOpen);
  m_shotLevelAct->setEnabled(m_proOpen);
  m_manShotSimAct->setEnabled(m_proOpen);
  m_sceneLevelAct->setEnabled(m_proOpen);
  m_episodeLevelAct->setEnabled(m_proOpen);
  m_seasonLevelAct->setEnabled(m_proOpen);
  m_manSceneAct->setEnabled(m_proOpen);
  m_summAct->setEnabled(m_proOpen);
  m_chartAct->setEnabled(m_proOpen);
}

void MainWindow::selectDefaultLevel()
{
  selectShotLevel();
}
