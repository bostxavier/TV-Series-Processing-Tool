#include <QProgressDialog>
#include <QLabel>
#include <QGridLayout>
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QElapsedTimer>

#include <qmath.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QDebug>
#include <iostream>

#include "MovieAnalyzer.h"
#include "ProjectModel.h"
#include "SpkInteractDialog.h"
#include "UtteranceTree.h"

using namespace cv;
using namespace std;

MovieAnalyzer::MovieAnalyzer(QWidget *parent)
  : QWidget(parent)
{
  m_vFrameProcessor = new VideoFrameProcessor;
  m_textProcessor = new TextProcessor;
  m_audioProcessor = new AudioProcessor;
  m_socialNetProcessor = new SocialNetProcessor;
  m_optimizer = new Optimizer;

  m_faceDetectProcess = new QProcess;

  connect(m_faceDetectProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(retrieveFaceBoundaries(int, QProcess::ExitStatus)));
  connect(m_faceDetectProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(faceDetectProcessOutput()));
  connect(m_audioProcessor, SIGNAL(musicRateRetrieved(QPair<qint64, qreal>)), this, SLOT(musicRateRetrieved(QPair<qint64, qreal>)));
}

MovieAnalyzer::~MovieAnalyzer()
{
}

QList<SpeechSegment *> MovieAnalyzer::denoiseSpeechSegments(QList<SpeechSegment *> speechSegments)
{
  return m_audioProcessor->denoiseSpeechSegments(speechSegments);
}

QList<SpeechSegment *> MovieAnalyzer::filterSpeechSegments(QList<SpeechSegment *> speechSegments)
{
  return m_audioProcessor->filterSpeechSegments(speechSegments);
}

QSize MovieAnalyzer::getResolution(const QString &fName)
{
  m_cap.release();
  m_cap.open(fName.toStdString());

  return QSize(m_cap.get(CV_CAP_PROP_FRAME_WIDTH), m_cap.get(CV_CAP_PROP_FRAME_HEIGHT));
}

qreal MovieAnalyzer::getFps(const QString &fName)
{
  m_cap.release();
  m_cap.open(fName.toStdString());

  // does not work since upgrading Ubuntu to 14.04
  // return m_cap.get(CV_CAP_PROP_FPS);

  return 25;
}

bool MovieAnalyzer::setShotCorrMatrix(arma::mat &D, const QString &fName, QList<Shot *> shots, int nV, int nH)
{
  m_cap.release();
  m_cap.open(fName.toStdString());

  // number of vertical and horizontal blocks
  int nVBlock(nV);
  int nHBlock(nH);

  // number of bins of HSV histograms
  int nHBins(24);
  int nSBins(8);
  int nVBins(64);

  // previous and current shot frame
  Mat prevShotFrame;
  Mat currShotFrame;

  // subimages of current and past frames
  QVector<Mat> prevBlocks;
  QVector<Mat> currBlocks;

  // local histogram of each frame block
  QVector<Mat> locHisto(nVBlock * nHBlock);

  // accumulator of previous lists of local histograms
  QList<QVector<Mat> > locHistoBuffer;

  // distance between first frame of current shot and last frame of past shot
  QVector<qreal> distance(nVBlock * nHBlock);

  // frame duration
  // doesn't work anymore after updating Ubuntu
  // int frameDur = 1 / m_cap.get(CV_CAP_PROP_FPS) * 1000;
  int frameDur = 1 / 25.0 * 1000;
  
  // distance between local histograms
  qreal locDistance;
  
  // activate appropriate metrics
  m_vFrameProcessor->activCorrel();

  // looping over shot positions
  for (int i(1); i < shots.size(); i++) {

    //////////////////////////////
    // processing previous shot //
    //////////////////////////////

    m_cap.set(CV_CAP_PROP_POS_MSEC, shots[i]->getPosition() - frameDur - 40);
    m_cap >> prevShotFrame;
    
    // retrieving previous shot frame blocks
    prevBlocks = m_vFrameProcessor->splitImage(prevShotFrame, nVBlock, nHBlock);

    // resizing local histograms vector if necessary
    if (prevBlocks.size() != nVBlock * nHBlock)
      locHisto.resize(prevBlocks.size());

    // looping over previous shot frame blocks
    for (int j(0); j < prevBlocks.size(); j++)

      // compute V/HS/HSV histogram for each block of previous shot last frame
      locHisto[j] = m_vFrameProcessor->genHsvHisto(prevBlocks[j], nHBins, nSBins, nVBins);
    
    // saving list of corresponding histograms
    locHistoBuffer.push_front(locHisto);

    /////////////////////////////
    // processing current shot //
    /////////////////////////////

    m_cap >> currShotFrame;

    // retrieving current shot frame blocks
    currBlocks = m_vFrameProcessor->splitImage(currShotFrame, nVBlock, nHBlock);

    // resizing local histograms vector if necessary
    if (currBlocks.size() != nVBlock * nHBlock) {
      locHisto.resize(currBlocks.size());
      distance.resize(currBlocks.size());
    }

    // looping over current shot frame blocks
    for (int j(0); j < currBlocks.size(); j++)

      // compute  V/HS/HSV histogram for each block of current shot last frame
      locHisto[j] = m_vFrameProcessor->genHsvHisto(currBlocks[j], nHBins, nSBins, nVBins);

    for (int j(0); j < locHistoBuffer.size(); j++) {
      
      // retrieving past list of local histograms
      QVector<Mat> prevLocHisto = locHistoBuffer[j];

      // looping over list of local histograms and computing local distances
      for (int k(0); k < prevLocHisto.size(); k++)
	distance[k] = m_vFrameProcessor->distanceFromPrev(locHisto[k], prevLocHisto[k]);

      // averaging local distances
      locDistance = meanDistance(distance);

      // updating shot correlation matrix
      D(i, i - j - 1) = locDistance;
      D(i - j - 1, i) = locDistance;
    }
  }

  return true;
}

bool MovieAnalyzer::extractShots(const QString &fName, int histoType, int nVBins, int nHBins, int nSBins, int metrics, qreal threshold1, qreal threshold2, int nVBlock, int nHBlock, bool viewProgress)
{
  m_cap.release();
  m_cap.open(fName.toStdString());

  // current frame
  Mat frame;

  // subimages of current frame
  QVector<Mat> blocks(nVBlock * nHBlock);

  // corresponding local histograms
  QVector<Mat> locHisto(nVBlock * nHBlock);

  // distances between current and previous local histograms
  QVector<qreal> distance(nVBlock * nHBlock);

  // distance between current and past frame
  qreal dist;

  // frames considered to detect cut
  QList<qreal> window;

  // current frame position and index
  qint64 position(0);
  int n(0);

  // shot beginning
  qint64 shotStart(0);

  // previous frame position
  qint64 prevPosition(0);

  // indicates that no more frame is available in video capture
  bool open(true);

  // normalized local copies of thresholds
  qreal thresh1(threshold1 / 100.0);
  qreal thresh2(threshold2 / 100.0);

  // activate appropriate metrics
  switch (metrics) {
  case 4:
    m_vFrameProcessor->activL1();
    break;
  case 5:
    m_vFrameProcessor->activL2();
    break;
  case CV_COMP_CORREL:
    m_vFrameProcessor->activCorrel();
    break;
  case CV_COMP_CHISQR:
    m_vFrameProcessor->activChiSqr();
    break;
  case CV_COMP_INTERSECT:
    m_vFrameProcessor->activIntersect();
    break;
  case CV_COMP_HELLINGER:
    m_vFrameProcessor->activHellinger();
    break;
  }
  
  // progress bar
  QProgressDialog progress(tr("Extracting shots..."), tr("Cancel"), 0, m_cap.get(CV_CAP_PROP_FRAME_COUNT), this);
  if (viewProgress) {
    progress.setWindowModality(Qt::WindowModal);
  }

  // beginning of first shot
  shotStart = m_cap.get(CV_CAP_PROP_POS_MSEC);
  
  while (open) {

    // retrieve index and position of next frame
    n = m_cap.get(CV_CAP_PROP_POS_FRAMES);
    position = m_cap.get(CV_CAP_PROP_POS_MSEC) + 40;
    
    open = m_cap.read(frame);

    // convert to HSV
    cvtColor(frame, frame, CV_BGR2HSV);
    
    if (open) {

      // retrieving previous shot frame blocks
      blocks = m_vFrameProcessor->splitImage(frame, nVBlock, nHBlock);

      // looping over frame blocks
      for (int i(0); i < blocks.size(); i++) {

	// compute  V/HS/HSV histogram for each frame block
	switch (histoType) {
	case 0:
	  locHisto[i] = m_vFrameProcessor->genVHisto(blocks[i], nVBins);
	  break;
	case 1:
	  locHisto[i] = m_vFrameProcessor->genHsHisto(blocks[i], nHBins, nSBins);
	  break;
	case 2:
	  locHisto[i] = m_vFrameProcessor->genHsvHisto(blocks[i], nHBins, nSBins, nVBins);
	  break;
	}

	// compute distance from previous frame for each block
	if (!m_prevLocHisto.isEmpty())
	  distance[i] = m_vFrameProcessor->distanceFromPrev(locHisto[i], m_prevLocHisto[i]);
      }
     
      dist = meanDistance(distance);
      
      /*** for displaying purpose ***/
      /*
      qDebug();
      for (int i(0); i < nHBlock; i++) {
	for (int j(0); j < nVBlock; j++) {
	  printf("%4.4f\t", 1 - distance[i * nVBlock + j]);
	}
	cout << endl;
      }

      qDebug();
      qDebug() << (1 - dist);
      */

      // append current distance to the window
      window.push_back(dist);

      if (window.size() > 3)
	window.pop_front();

      // insert shot at current position
      if (window[0] <= thresh2 && window[1] >= thresh1 && window[2] <= thresh2) {
	emit insertShot(shotStart, prevPosition);
	shotStart = prevPosition;
      }

      // updating previous local histograms to current ones
      m_prevLocHisto.resize(blocks.size());
      m_prevLocHisto = locHisto;

      // updating previous position to current
      prevPosition = position;
    }

    // update progress bar
    if (viewProgress) {
      progress.setValue(n);
      if (progress.wasCanceled())
	return false;
    }
  }

  emit insertShot(shotStart, position);

  return true;
}

bool MovieAnalyzer::labelSimilarShots(QString fName, int histoType, int nVBins, int nHBins, int nSBins, int metrics, qreal maxDist, int windowSize, QList<Shot *> shots, int nVBlock, int nHBlock, bool viewProgress)
{
  m_cap.release();
  m_cap.open(fName.toStdString());

  // number of shots
  int n(shots.size());

  // previous and current shot frame
  Mat prevShotFrame;
  Mat currShotFrame;

  // subimages of current and past frames
  QVector<Mat> prevBlocks;
  QVector<Mat> currBlocks;

  // local histogram of each frame block
  QVector<Mat> locHisto(nVBlock * nHBlock);

  // accumulator of previous lists of local histograms
  QList<QVector<Mat> > locHistoBuffer;

  // distance between first frame of current shot and last frame of past shot
  QVector<qreal> distance(nVBlock * nHBlock);

  /*** for displaying purpose ***/
  QList<Mat> frameBuffer;

  // frame duration
  // doesn't work anymore after updating Ubuntu
  // int frameDur = 1 / m_cap.get(CV_CAP_PROP_FPS) * 1000;
  int frameDur = 1 / 25.0 * 1000;
  
  // distance between local histograms
  qreal locDistance;
  // minimum distance from current frame observed so far
  qreal minDistance;
  // corresponding index
  int jMin;

  // current camera label
  int nCamera(0);
  
  // camera label for each shot
  QVector<int> shotCamera(n);

  // activate appropriate metrics
  switch (metrics) {
  case 4:
    m_vFrameProcessor->activL1();
    break;
  case 5:
    m_vFrameProcessor->activL2();
    break;
  case CV_COMP_CORREL:
    m_vFrameProcessor->activCorrel();
    break;
  case CV_COMP_CHISQR:
    m_vFrameProcessor->activChiSqr();
    break;
  case CV_COMP_INTERSECT:
    m_vFrameProcessor->activIntersect();
    break;
  case CV_COMP_HELLINGER:
    m_vFrameProcessor->activHellinger();
    break;
  }

  // progress bar
  QProgressDialog progress(tr("Retrieving similar shots..."), tr("Cancel"), 0, n, this);
  int count(0);

  if (viewProgress)
    progress.setWindowModality(Qt::WindowModal);

  // normalizing max distance required to link two shots
  maxDist /= 100.0;

  // looping over shot positions
  shotCamera[0] = nCamera;
  shots[0]->setCamera(shotCamera[0], Segment::Automatic);

  for (int i(1); i < n; i++) {
    
    /****************************/
    /* processing previous shot */
    /****************************/

    m_cap.set(CV_CAP_PROP_POS_MSEC, shots[i]->getPosition() - frameDur - 40);
    m_cap >> prevShotFrame;
    
    // retrieving previous shot frame blocks
    prevBlocks = m_vFrameProcessor->splitImage(prevShotFrame, nVBlock, nHBlock);

    // resizing local histograms vector if necessary
    if (prevBlocks.size() != nVBlock * nHBlock)
      locHisto.resize(prevBlocks.size());

    // looping over previous shot frame blocks
    for (int j(0); j < prevBlocks.size(); j++)

      // compute  V/HS/HSV histogram for each block of previous shot last frame
      switch (histoType) {
      case 0:
	locHisto[j] = m_vFrameProcessor->genVHisto(prevBlocks[j], nVBins);
	break;
      case 1:
	locHisto[j] = m_vFrameProcessor->genHsHisto(prevBlocks[j], nHBins, nSBins);
	break;
      case 2:
	locHisto[j] = m_vFrameProcessor->genHsvHisto(prevBlocks[j], nHBins, nSBins, nVBins);
       break;
      }

    /*** for displaying purpose ***/
    /* Mat frameCopy;
    prevShotFrame.copyTo(frameCopy);
    frameBuffer.push_front(frameCopy);
    if (frameBuffer.size() == windowSize + 1)
    frameBuffer.pop_back(); */

    // saving list of corresponding histograms
    locHistoBuffer.push_front(locHisto);

    // update buffer boundaries
    if (locHistoBuffer.size() == windowSize + 1)
      locHistoBuffer.pop_back();
    
    /***************************/
    /* processing current shot */
    /***************************/

    m_cap >> currShotFrame;

    /*** for displaying purpose ***/
    // imshow("present", currShotFrame);

    // retrieving current shot frame blocks
    currBlocks = m_vFrameProcessor->splitImage(currShotFrame, nVBlock, nHBlock);

    // resizing local histograms vector if necessary
    if (currBlocks.size() != nVBlock * nHBlock) {
      locHisto.resize(currBlocks.size());
      distance.resize(currBlocks.size());
    }

    // looping over current shot frame blocks
    for (int j(0); j < currBlocks.size(); j++)

      // compute  V/HS/HSV histogram for each block of current shot last frame
      switch (histoType) {
      case 0:
	locHisto[j] = m_vFrameProcessor->genVHisto(currBlocks[j], nVBins);
	break;
      case 1:
	locHisto[j] = m_vFrameProcessor->genHsHisto(currBlocks[j], nHBins, nSBins);
	break;
      case 2:
	locHisto[j] = m_vFrameProcessor->genHsvHisto(currBlocks[j], nHBins, nSBins, nVBins);
       break;
      }

    // compute distance between current and past frame local histograms
    minDistance = 1.0;
    jMin = 0;

    for (int j(0); j < locHistoBuffer.size(); j++) {
      
      // retrieving past list of local histograms
      QVector<Mat> prevLocHisto = locHistoBuffer[j];

      // looping over list of local histograms and computing local distances
      for (int k(0); k < prevLocHisto.size(); k++)
	distance[k] = m_vFrameProcessor->distanceFromPrev(locHisto[k], prevLocHisto[k]);

      // averaging local distances
      locDistance = meanDistance(distance);

      /*** for displaying purpose ***/
      /* qDebug();
      
      for (int k(0); k < nHBlock; k++) {
	for (int l(0); l < nVBlock; l++) {
	  printf("%4.4f\t", 1 - distance[k * nVBlock + l]);
	}
	cout << endl;
      }

      qDebug();
      qDebug() << (1 - locDistance);
      imshow("past", frameBuffer[j]);
      waitKey(0); */
      
      if (locDistance < minDistance) {
	minDistance = locDistance;
	jMin = j;
      }
    }

    // similar shot retrieved among previous shots
    if (minDistance <= maxDist) {
      int iSim = i - (jMin + 1);
      shotCamera[i] = shotCamera[iSim];
      shots[i]->setCamera(shotCamera[i], Segment::Automatic);
    }

    // no similar shot retrieved: new camera label
    else {
      shotCamera[i] = nCamera++;
      shots[i]->setCamera(shotCamera[i], Segment::Automatic);
    }

    // updating progress bar
    if (viewProgress) {
      progress.setValue(++count);
      if (progress.wasCanceled())
	return false;
    }
  }
      
  return true;
}

bool MovieAnalyzer::faceDetectionOpenCV(QList<Shot *> shots, const QString &fName, int minHeight)
{
  m_cap.release();
  m_cap.open(fName.toStdString());

  // current frame
  Mat frame;

  // frame height
  int absMinHeight;

  // current frame position and index
  qint64 position(0);

  // indicates that no more frame is available in video capture
  bool open(true);
  
  // progress bar
  QProgressDialog progress(tr("Detecting faces..."), tr("Cancel"), 0, shots.size(), this);
  progress.setWindowModality(Qt::WindowModal);
  
  // initializing cascade classifier (needed by OpenCV face detector)
  QString faceCascadeName = "faceDetection/model/haarcascade_frontalface_alt.xml";
  CascadeClassifier faceCascade;

  if (!faceCascade.load(faceCascadeName.toStdString()))
    return false;
  
  // looping over shots
  for (int i(0); i < shots.size(); i++) {
    
    // set video capture at shot beginning
    position = shots[i]->getPosition();
    m_cap.set(CV_CAP_PROP_POS_MSEC, position);

    open = m_cap.read(frame);

    // frame found
    if (open) {
    
      absMinHeight = static_cast<int>(frame.rows * minHeight / 100.0);
      QList<QRect> frameFaces;
      
      frameFaces = detectFacesOpenCV(faceCascade, frame, absMinHeight);

      if (frameFaces.size() > 0) {
	emit setCurrShot(position);
	emit appendFaces(position, frameFaces);
      }
    }
    
    // update progress bar
    progress.setValue(i);
    if (progress.wasCanceled())
      return false;
  }
  
  return true;
}

QList<QRect> MovieAnalyzer::detectFacesOpenCV(cv::CascadeClassifier &faceCascade, cv::Mat &frame, int minHeight)
{
  QList<QRect> faces;
  std::vector<Rect> facesRect;
  Mat frameGray;

  cvtColor(frame, frameGray, COLOR_BGR2GRAY);
  equalizeHist(frameGray, frameGray);
  
  // detecting faces
  faceCascade.detectMultiScale(frameGray, facesRect, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(minHeight, minHeight));

  for (size_t i = 0; i < facesRect.size(); i++ )
    faces.push_back(QRect(facesRect[i].x, facesRect[i].y, facesRect[i].width, facesRect[i].height));

  return faces;
}

void MovieAnalyzer::faceDetectionZhu(QList<Shot *> shots, const QString &fName, int minHeight)
{
  m_cap.release();
  m_cap.open(fName.toStdString());

  // current frame
  Mat frame;

  // frame height
  int absMinHeight;

  // current frame position and index
  qint64 position(0);

  // indicates that no more frame is available in video capture
  bool open(true);
  
  
  ////////////////////////
  // writing out frames //
  ////////////////////////
  
  // computing and writing out scaling factor
  qreal scaleFac = 80.0 / (minHeight * m_cap.get(CV_CAP_PROP_FRAME_HEIGHT) / 100.0);
  QFile scaleFile("faceDetection/tmp/scaleFactor.csv");

  if (!scaleFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QTextStream scaleOut(&scaleFile);
  scaleOut << scaleFac << endl;

  // looping over shots
  // for (int i(0); i < shots.size(); i++) {
  for (int i(0); i < 21; i++) {
    
    // set video capture at shot beginning
    position = shots[i]->getPosition();
    m_cap.set(CV_CAP_PROP_POS_MSEC, position);

    open = m_cap.read(frame);

    // frame found
    if (open) {
      
      // rescale and write out frame
      cv::resize(frame, frame, Size(), scaleFac, scaleFac);
      QString imgFName = "faceDetection/data/jpeg/" + QString::number(position) + ".jpg";
      imwrite(imgFName.toStdString(), frame);
      qDebug() << "extracting frame at" << position << "ms...";
    }
  }

  /////////////////////
  // detecting faces //
  /////////////////////

  QString program;
  QStringList arguments;

  program = "sh";
  arguments << "faceDetection/RUN_face_detection.sh";

  m_faceDetectProcess->start(program, arguments);
  m_faceDetectProcess->waitForFinished();
}

void MovieAnalyzer::faceDetectProcessOutput() 
{
  char buf[1024];
  qint64 lineLength = m_faceDetectProcess->readLine(buf, sizeof(buf));

  if (lineLength != -1) {
    QString outputText = QString::fromLocal8Bit(buf);
    qDebug() << outputText;
  }
}

void MovieAnalyzer::retrieveFaceBoundaries(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);

  if (exitStatus == QProcess::NormalExit) {
    qDebug() << "retrieving face boundaries...";
    emit externFaceDetection("faceDetection/output/bound/output.csv");
  }
}

void MovieAnalyzer::musicRateRetrieved(QPair<qint64, qreal> musicRate)
{
  emit setCurrShot(musicRate.first);
  emit insertMusicRate(musicRate);
}

bool MovieAnalyzer::localSpkDiarHC(UtteranceTree::DistType dist, bool norm, UtteranceTree::AgrCrit agr, UtteranceTree::PartMeth partMeth, bool weight, bool sigma, QList<SpeechSegment *> speechSegments, QList<QList<SpeechSegment *> > lsuSpeechSegments)
{
  arma::mat X;
  arma::mat Sigma;
  arma::mat W;
  arma::mat CovInv;

  m_audioProcessor->extractIVectors(speechSegments);
  X = m_audioProcessor->getEpisodeIVectors(speechSegments);
  Sigma = m_audioProcessor->genSigmaMat();
  W = m_audioProcessor->genWMat();

  UtteranceTree tree;                   // dendogram corresponding to local clustering  
  QString pattLabel;                    // pattern label
  QList<QPair<qreal, qreal> > localDer; // DER for each pattern

  // parameterizing tree
  tree.setDist(dist);
  tree.setAgr(agr);
  tree.setPartMeth(partMeth);

  // setting covariance matrix
  if (sigma)
    CovInv = pinv(Sigma);
  else
    CovInv = pinv(W);

  // normalize utterance vectors if necessary
  if (norm)
    normalize(X, CovInv, dist);
  
  // looping over shot patterns for labelling purpose
  for (int i(0); i < lsuSpeechSegments.size(); i++) {

    // number of instances
    int m = lsuSpeechSegments[i].size();
    
    // instance set contains at least two utterances
    if (m > 1) {

      // indices of utterances in current pattern
      arma::umat V(1, m);

      for (int j(0); j < m; j++)
	V(0, j) = speechSegments.indexOf(lsuSpeechSegments[i][j]);

      // matrix containing utterance i-vectors for current pattern
      arma::mat S = X.rows(V);

      // weighting utterances
      arma::mat W(1, m, arma::fill::ones);

      if (weight)
	for (int j(0); j < m; j++) {
	  qreal duration =
	  (lsuSpeechSegments[i][j]->getEnd() -
	   lsuSpeechSegments[i][j]->getPosition()) /
	    1000.0;
	  W(0, j) = duration;
	}
    
      W /= arma::accu(W);

      // setting tree
      tree.setTree(S, W, CovInv);

      // retrieving optimal partition
      QList<QList<int> > partition(tree.getPartition());

      for (int i(0); i < partition.size(); i++)
	for (int j(0); j < partition[i].size(); j++)
	  partition[i][j] = V(0, partition[i][j]);

      pattLabel = "LSU" + QString::number(i);

      QPair<qreal, qreal> res(computeSpkError(partition, speechSegments, pattLabel));
      localDer.push_back(res);
    }
  }

  // retrieving single-show Diarization Error Rate
  m_localDer = QString::number((retrieveSSDer(localDer)), 'g', 4);
  qDebug() << "Single-show DER:" << m_localDer << "%";

  return true;
}

bool MovieAnalyzer::globalSpkDiar(const QString &baseName, QList<QPair<qint64, qint64> > &subBound, QList<QString> &refLbl)
{
  Q_UNUSED(baseName);
  Q_UNUSED(subBound);
  Q_UNUSED(refLbl);
  
  /*
  UtteranceTree tree;                    // dendogram corresponding to global clustering  
  QList<QList<int> > partition;           // selected partition of the tree
  QList<QPair<int, qreal> > spkFeatures;  // speakers references and weights
  qreal weight;
  QMap<QString, qreal> spkWeight;
  QString speaker;

  m_baseName = baseName;
  m_subBound = subBound;
  m_refLbl = refLbl;

  // weighting speakers
  QMap<QString, QList<QPair<qreal, qreal> > >::const_iterator it = m_utterances.begin();

  while (it != m_utterances.end()) {

    speaker = it.key();
    QList<QPair<qreal, qreal> > utterances = it.value();
    
    spkWeight[speaker] = 0.0;

    for (int i(0); i < utterances.size(); i++)
      spkWeight[speaker] += utterances[i].second - utterances[i].first;

    it++;
  }

  // setting features
  it = m_utterances.begin();
  spkFeatures.clear();

  while (it != m_utterances.end()) {

    speaker = it.key();
    weight = spkWeight[speaker];
    spkFeatures.push_back(QPair<int, qreal>(m_speakers.indexOf(speaker), weight));

    it++;
  }

  // sending parameters to tree monitor
  m_treeMonitor = new SpkDiarMonitor(900, 300, true);
  m_treeMonitor->setDiarData(E, Sigma, CovW);
  m_treeMonitor->setSpeakers(m_speakers, spkWeight);
  m_treeMonitor->getCurrentPattern(spkFeatures);
  m_treeMonitor->show();

  // receiving playing signal from dendogram widget
  connect(m_treeMonitor, SIGNAL(playSub(QList<int>)), this, SLOT(playSpeakers(QList<int>)));

  // get partition for default parameters
  connect(m_treeMonitor, SIGNAL(setSpeakerPartition(QList<QList<int>>)), this, SLOT(setSpeakerPartition(QList<QList<int>>)));

  // sending scores obtained to diarization monitor
  connect(this, SIGNAL(setLocalDer(const QString &)), m_treeMonitor, SLOT(setLocalDer(const QString &)));
  connect(this, SIGNAL(setGlobalDer(const QString &)), m_treeMonitor, SLOT(setGlobalDer(const QString &)));
  
  m_treeMonitor->exportResults();
  */

  return true;
}

void MovieAnalyzer::setCoOccurrInteract(QList<QList<SpeechSegment *> > unitSpeechSegments, int nbDiscards)
{
  // looping over speech segments by unit
  for (int i(0); i < unitSpeechSegments.size(); i++) {
    
    QStringList speakers;
    QMap<QString, QStringList> coOccSpeakers;

    // first loop over speech segments
    for (int j(0); j < unitSpeechSegments[i].size() - nbDiscards; j++) {

      // current speaker
      QString currSpk = unitSpeechSegments[i][j]->getLabel(Segment::Manual);

      // possibly add new speaker to co-occurring ones
      if (!speakers.contains(currSpk))
	speakers.push_back(currSpk);
    }

    // setting co-occurring speakers
    qSort(speakers);
    for (int j(0); j < speakers.size(); j++)
      for (int k(0); k < speakers.size(); k++)
	if (j != k)
	  coOccSpeakers[speakers[j]].push_back(speakers[k]);

    // assign interlocutors to each segment
    for (int j(0); j < unitSpeechSegments[i].size(); j++) {

      // current speaker
      QString currSpk = unitSpeechSegments[i][j]->getLabel(Segment::Manual);

      // set interlocutors for current segment
      unitSpeechSegments[i][j]->setInterLocs(coOccSpeakers[currSpk], SpkInteractDialog::CoOccurr);
    }
  }
}
 
void MovieAnalyzer::setSequentialInteract(QList<QList<SpeechSegment *> > unitSpeechSegments, int nbDiscards, int interThresh, const QVector<bool> &rules)
{
  // convert interaction threshold in ms
  interThresh *= 1000;

  // looping over units
  for (int i(0); i < unitSpeechSegments.size(); i++) {

    QList<QList<SpeechSegment *> > unitSpeechSeg;
    QList<SpeechSegment *> currSpeechSeg;
    QString prevSpeaker("");

    // looping over speech segments
    for (int j(0); j < unitSpeechSegments[i].size() - nbDiscards; j++) {

      // current speaker
      QString currSpeaker = unitSpeechSegments[i][j]->getLabel(Segment::Manual);

      // new speaker: push gathered segment
      if (prevSpeaker != "" && prevSpeaker != currSpeaker) {
	unitSpeechSeg.push_back(currSpeechSeg);
	currSpeechSeg.clear();
      }

      currSpeechSeg.push_back(unitSpeechSegments[i][j]);
      prevSpeaker = currSpeaker;
    }

    // processing last gathered segment
    if (!currSpeechSeg.isEmpty())
      unitSpeechSeg.push_back(currSpeechSeg);

    // consider only scenes with more than one speaker
    if (unitSpeechSeg.size() > 1) {

      // looping over gathered speech segments
      for (int j(0); j < unitSpeechSeg.size(); j++) {

	// speaker and boundaries of current segment
	QString currSpeaker = unitSpeechSeg[j].first()->getLabel(Segment::Manual);
	qint64 position = unitSpeechSeg[j].first()->getPosition();
	qint64 end = unitSpeechSeg[j].last()->getEnd();
	
	bool sameSurround = sameSurroundSpeaker(j, unitSpeechSeg);

	// sequence (2a): | spk_1 -> spk_2
	if (j == 0) {

	  QString nextSpeaker = unitSpeechSeg[j+1].first()->getLabel(Segment::Manual);
	  qint64 nextPosition = unitSpeechSeg[j+1].first()->getPosition();
	
	  if (nextPosition - end <= interThresh)
	    for (int k(0); k < unitSpeechSeg[j].size(); k++)
	      if (rules[1])
		unitSpeechSeg[j][k]->addInterLoc(nextSpeaker, SpkInteractDialog::Sequential);
	}

	// sequence (2b): spk_1 <- spk_2 |
	else if (j == unitSpeechSeg.size() - 1) {

	  QString prevSpeaker = unitSpeechSeg[j-1].last()->getLabel(Segment::Manual);
	  qint64 prevEnd = unitSpeechSeg[j-1].last()->getEnd();
	
	  if (position - prevEnd <= interThresh)
	    for (int k(0); k < unitSpeechSeg[j].size(); k++)
	      if (rules[1])
		unitSpeechSeg[j][k]->addInterLoc(prevSpeaker, SpkInteractDialog::Sequential);
	}

	// sequence (1): spk_1 <- spk_2 -> spk_1
	else if (sameSurround) {

	  QString prevSpeaker = unitSpeechSeg[j-1].last()->getLabel(Segment::Manual);
	  qint64 prevEnd = unitSpeechSeg[j-1].last()->getEnd();
	  qint64 nextPosition = unitSpeechSeg[j+1].first()->getPosition();
	
	  if (position - prevEnd <= interThresh || nextPosition - end <= interThresh)
	    for (int k(0); k < unitSpeechSeg[j].size(); k++) 
	      if (rules[0]) {
		unitSpeechSeg[j][k]->addInterLoc(prevSpeaker, SpkInteractDialog::Sequential);
		// qDebug() << i << j << k << currSpeaker << "->" << prevSpeaker;
	    }
	}

	else {
	  bool prevOcc = interPrevOcc(j, unitSpeechSeg);
	  bool nextOcc = interNextOcc(j, unitSpeechSeg);

	  QString prevSpeaker = unitSpeechSeg[j-1].last()->getLabel(Segment::Manual);
	  QString nextSpeaker = unitSpeechSeg[j+1].first()->getLabel(Segment::Manual);
	  qint64 prevEnd = unitSpeechSeg[j-1].last()->getEnd();
	  qint64 nextPosition = unitSpeechSeg[j+1].first()->getPosition();

	  // sequence (3a): (spk_2) - spk_1 <- spk_2 - spk_3
	  if (prevOcc && !nextOcc && position - prevEnd <= interThresh) {
	    for (int k(0); k < unitSpeechSeg[j].size(); k++)
	      if (rules[2])
		unitSpeechSeg[j][k]->addInterLoc(prevSpeaker, SpkInteractDialog::Sequential);
	  }

	  // sequence (3b): spk_1 - spk_2 -> spk_3 - (spk_2)
	  else if (!prevOcc && nextOcc && nextPosition - end <= interThresh) {
	    for (int k(0); k < unitSpeechSeg[j].size(); k++)
	      if (rules[2])
		unitSpeechSeg[j][k]->addInterLoc(nextSpeaker, SpkInteractDialog::Sequential);
	  }

	  // sequence (4): (spk2) - spk_1 <- spk_2 -> spk_3 - (spk_2)
	  else if (rules[3]) {
	    qint64 lim = static_cast<qint64>((nextPosition + prevEnd) / 2.0);
	    for (int k(0); k < unitSpeechSeg[j].size(); k++) {
	      qint64 center = static_cast<qint64>((unitSpeechSeg[j][k]->getPosition() + unitSpeechSeg[j][k]->getEnd()) / 2.0);
	      if (center <= lim && position - prevEnd <= interThresh)
		unitSpeechSeg[j][k]->addInterLoc(prevSpeaker, SpkInteractDialog::Sequential);
	      else if (center > lim && nextPosition - end <= interThresh)
		unitSpeechSeg[j][k]->addInterLoc(nextSpeaker, SpkInteractDialog::Sequential);
	    }
	  }
	}
      }
    }
  }
}

bool MovieAnalyzer::sameSurroundSpeaker(int i, QList<QList<SpeechSegment *> > speechSegments)
{
  bool sameSurround(false);
  int iPrev(i-1);
  int iNext(i+1);
  QString prevSpeaker;
  QString currSpeaker;
  QString nextSpeaker;

  if (iPrev >= 0 && iNext < speechSegments.size()) {
    prevSpeaker = speechSegments[iPrev][0]->getLabel(Segment::Manual);
    currSpeaker = speechSegments[i][0]->getLabel(Segment::Manual);
    nextSpeaker = speechSegments[iNext][0]->getLabel(Segment::Manual);
    sameSurround = (currSpeaker != prevSpeaker && prevSpeaker == nextSpeaker);
  }

  return sameSurround;
}

bool MovieAnalyzer::interPrevOcc(int i, QList<QList<SpeechSegment *> > speechSegments)
{
  bool prevOcc(false);

  if (i >= 2)
    prevOcc = sameSurroundSpeaker(i-1, speechSegments);

  return prevOcc;
}
 
bool MovieAnalyzer::interNextOcc(int i, QList<QList<SpeechSegment *> > speechSegments)
{
  bool nextOcc(false);

  if (i < speechSegments.size() - 2)
    nextOcc = sameSurroundSpeaker(i+1, speechSegments);

  return nextOcc;
}

void MovieAnalyzer::musicTracking(const QString &epFName, int frameRate, int mtWindowSize, int mtHopSize, int chromaStaticFrameSize, int chromaDynamicFrameSize)
{
  m_audioProcessor->trackMusic(epFName, frameRate, mtWindowSize, mtHopSize, chromaStaticFrameSize, chromaDynamicFrameSize);
}

void MovieAnalyzer::summarization(SummarizationDialog::Method method, int seasonNb, const QString &speaker, int dur, qreal granu, QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments)
{
  QElapsedTimer timer;
  timer.start();
  
  qreal totDuration = getTotalSceneDuration(speaker, sceneSpeechSegments);
  qreal summDuration(0.0);

  // Segments to play along with episode index
  QList<QPair<Episode *, QPair<qint64, qint64> > > segments;

  // steps of character's storyline
  QList<int> steps;

  // speaker-oriented summary
  if (speaker != "") {

    // only consider LSUs involving targetted speaker
    filterLsus(speaker, lsuShots, lsuSpeechSegments);

    // split character storyline
    QMap<int, int> sceneMapping;
    QList<QPair<QMap<QString, qreal>, QList<int> > > sceneCovering = splitStoryLine(speaker, granu, sceneSpeechSegments, sceneMapping);

    // retrieve LSU features
    QList<qreal> lsuDuration = getLsuDuration(lsuShots, lsuSpeechSegments);
    arma::mat FS = computeLsuFormalSimMat(lsuShots);

    switch (method) {
      
    case SummarizationDialog::Content:
      segments = fullSpeakerSummary(sceneSpeechSegments, lsuShots, lsuSpeechSegments, sceneCovering, speaker, lsuDuration, FS, dur, 0.0, steps, sceneMapping);
      break;

    case SummarizationDialog::Style:
      segments = styleSpeakerSummary(sceneSpeechSegments, lsuShots, lsuSpeechSegments, sceneCovering.size() * dur, lsuDuration, FS, sceneMapping, speaker);
      break;

    case SummarizationDialog::Both:
      segments = fullSpeakerSummary(sceneSpeechSegments, lsuShots, lsuSpeechSegments, sceneCovering, speaker, lsuDuration, FS, dur, 4.0, steps, sceneMapping);
      break;

    case SummarizationDialog::Random:
      segments = randomSpeakerSummary(lsuShots, lsuSpeechSegments, sceneCovering.size() * dur, lsuDuration, FS);
      break;
    }

    // compute summary duration
    for (int i(0); i < segments.size(); i++) {
      qreal duration = (segments[i].second.second - segments[i].second.first) / 1000.0;
      summDuration += duration;
    }
  }

  qDebug() << "Summary generated in " << QString::number((double) timer.elapsed()/1000, 'f', 3) << "seconds";
  
  qreal compRate = totDuration / summDuration;

  qDebug() << "****************************";
  qDebug() << "* Compression rate: " << QString::number(compRate, 'g', 3);
  qDebug() << "*    (" << summDuration << "/" << totDuration << ")  *";
  qDebug() << "****************************";

  // filter segments untill selected season
  QList<QPair<Episode *, QPair<qint64, qint64> > > summary;

  for (int i(0); i < segments.size(); i++) {
    Episode *currEpisode = segments[i].first;
    int currSeasNbr = currEpisode->parent()->row();
    if (currSeasNbr <= seasonNb)
      summary.push_back(segments[i]);
  }

  exportToFile(summary, speaker, seasonNb, method, steps);

  emit playLsus(summary);
}

void MovieAnalyzer::exportToFile(const QList<QPair<Episode *, QPair<qint64, qint64> > > summary, QString speaker, int seasNbr, SummarizationDialog::Method method, const QList<int> &steps)
{
  speaker = speaker.toLower();
  speaker.replace(" ", "_");

  QString fName = speaker + "_" + QString::number(seasNbr+1);
  switch (method) {
      
    case SummarizationDialog::Content:
      fName += "_cnt.csv";
      break;

    case SummarizationDialog::Style:
      fName += "_sty.csv";
      break;

    case SummarizationDialog::Both:
      fName += "_full.csv";
      break;

    case SummarizationDialog::Random:
      fName += "_rnd.csv";
      break;
    }

  QFile summFile("/home/xavier/Dropbox/tv_series_proc_tool/tools/gen_summary/lst/" + fName);

  if (!summFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QTextStream summOut(&summFile);

  for (int i(0); i < summary.size(); i++) {

    QString epFName = summary[i].first->getFName();
    qreal start = summary[i].second.first / 1000.0;
    qreal dur = (summary[i].second.second - summary[i].second.first) / 1000.0;
    summOut << epFName << "\t" << start << "\t" << dur;
    if (!steps.isEmpty())
      summOut << "\t" << steps[i];
    summOut << endl;
  }

  qDebug() << fName;
}

qreal MovieAnalyzer::getTotalSceneDuration(const QString &speaker, QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  qreal totDuration(0.0);

  for (int i(0); i < sceneSpeechSegments.size(); i++)
    if (!sceneSpeechSegments[i].isEmpty()) {

      qreal duration = (sceneSpeechSegments[i].last()->getEnd() - sceneSpeechSegments[i].first()->getPosition()) / 1000.0;

      if (speaker == "")
	totDuration += duration;
      
      else {
	int j(0);
	while (j < sceneSpeechSegments[i].size() && sceneSpeechSegments[i][j]->getLabel(Segment::Manual) != speaker)
	  j++;
	if (j < sceneSpeechSegments[i].size())
	  totDuration += duration;
      }
    }

  return totDuration;
}

QList<QPair<Episode *, QPair<qint64, qint64> > > MovieAnalyzer::styleSpeakerSummary(QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments, int dur, const QList<qreal> &lsuDuration, const arma::mat &FS, const QMap<int, int> &sceneMapping, const QString &speaker)
{
  QList<int> spkSceneSelected;
  QList<QPair<Episode *, QPair<qint64, qint64> > > summary;
  QList<int> steps;

  bool diff(false);

  // LSU(s) / scene
  QMap<int, QList<int> > sceneLsus = retrieveSceneLsus(sceneSpeechSegments, lsuSpeechSegments);
  QMap<int, int> lsuScene;

  QMap<int, QList<int> >::const_iterator it = sceneLsus.begin();
  while (it != sceneLsus.end()) {

    int sceneIdx = it.key();
    QList<int> lsuIndices = it.value();

    for (int i(0); i < lsuIndices.size(); i++)
      lsuScene[lsuIndices[i]] = sceneIdx;

    it++;
  }

  // retrieve LSU features
  QList<qreal> lsuShotSize = getLsuShotSize(lsuShots, diff);
  QList<qreal> lsuMusicRate = getLsuMusicRate(lsuShots, diff);
  QList<qreal> lsuSocialRelevance;
  for (int i(0); i < lsuShotSize.size(); i++)
    lsuSocialRelevance.push_back(0.0);

  // parameters of LSUs selection
  arma::vec P = computeCosts(lsuShotSize, lsuMusicRate, lsuSocialRelevance, {1.0, 1.0, 0.0});
  arma::vec W = computeWeights(lsuDuration);

  // social dissimilarity between LSUs
  arma::mat SD;
  SD.zeros(P.n_rows, P.n_rows);

  // select most expressive LSUs
  QList<int> selected;
  m_optimizer->mmr(P, W, FS, SD, dur, selected);

  // build summary
  for (int i(0); i < selected.size(); i++) {

    qDebug() << selected[i] << P(selected[i]) << W(selected[i]) << ":" << lsuShotSize[selected[i]] << lsuMusicRate[selected[i]] << lsuSocialRelevance[selected[i]] << endl;
    displayLsuContent(lsuShots[selected[i]], lsuSpeechSegments[selected[i]]);
    qDebug();

    // segment boundaries
    QPair<qint64, qint64> boundaries = getLsuBound(selected[i], lsuShots, lsuSpeechSegments);
	
    // current episode
    Shot *first = lsuShots[selected[i]].first();
    Episode *episode = dynamic_cast<Episode *>(first->parent()->parent());

    // possibly merge current and previous segments if consecutive
    if (!summary.isEmpty()) {
      Episode *prevEpisode = summary.last().first;
      QPair<qint64, qint64> prevBound = summary.last().second;
      if (episode == prevEpisode && boundaries.first <= prevBound.second) {
	summary.pop_back();
	boundaries.first = prevBound.first;
      }
    }
	
    summary.push_back(QPair<Episode *, QPair<qint64, qint64> >(episode, boundaries));

    // write out scene of selected LSUs
    int lsuIdx = selected[i];
    int lsuSceneIdx = lsuScene[lsuIdx];
    int spkSceneIdx = sceneMapping[lsuSceneIdx];
    qDebug() << lsuIdx << lsuSceneIdx << spkSceneIdx;
    
    if (sceneMapping.contains(lsuSceneIdx))
	spkSceneSelected.push_back(spkSceneIdx);
  }


  arma::uvec S;
  S.zeros(spkSceneSelected.size());
  for (int i(0); i < spkSceneSelected.size(); i++)
    S(i) = spkSceneSelected[i];

  // QString dir = "/home/xavier/Dropbox/tv_series_proc_tool/tools/sna/matlab/data/";
  // QString fName = dir + speaker.toUpper().replace(" ", "_") + "_SS.dat";

  // S.save(fName.toStdString(), arma::raw_ascii);

  qDebug() << "# candidate LSUs:" << W.n_rows;
  qDebug() << "avg. dur.:" << mean(W) << "s.";
  qDebug() << "# selected LSUs:" << selected.size();
  qDebug() << "avg. dur.:" << getAvgDuration(selected, lsuDuration) << "s.";

  return summary;
}

QList<QPair<Episode *, QPair<qint64, qint64> > > MovieAnalyzer::randomSpeakerSummary(QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments, int dur, const QList<qreal> &lsuDuration, const arma::mat &FS)
{
  QList<QPair<Episode *, QPair<qint64, qint64> > > summary;

  // parameters of LSUs selection
  arma::vec W = computeWeights(lsuDuration);

  // randomly select LSUs
  QList<int> selected;
  m_optimizer->randomSelection(W, FS, dur, selected);

  // build summary
  for (int i(0); i < selected.size(); i++) {

    qDebug() << selected[i] << W(selected[i]) << endl;
    displayLsuContent(lsuShots[selected[i]], lsuSpeechSegments[selected[i]]);
    qDebug();

    // segment boundaries
    QPair<qint64, qint64> boundaries = getLsuBound(selected[i], lsuShots, lsuSpeechSegments);
	
    // current episode
    Shot *first = lsuShots[selected[i]].first();
    Episode *episode = dynamic_cast<Episode *>(first->parent()->parent());

    // possibly merge current and previous segments if consecutive
    if (!summary.isEmpty()) {
      Episode *prevEpisode = summary.last().first;
      QPair<qint64, qint64> prevBound = summary.last().second;
      if (episode == prevEpisode && boundaries.first <= prevBound.second) {
	summary.pop_back();
	boundaries.first = prevBound.first;
      }
    }
	
    summary.push_back(QPair<Episode *, QPair<qint64, qint64> >(episode, boundaries));
  }

  return summary;
}

QList<QPair<Episode *, QPair<qint64, qint64> > > MovieAnalyzer::fullSpeakerSummary(QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments, const QList<QPair<QMap<QString, qreal>, QList<int> > > &sceneCovering, const QString &speaker, const QList<qreal> &lsuDuration, const arma::mat &FS, int dur, qreal styleWeight, QList<int> &steps, const QMap<int, int> &sceneMapping)
{
  QList<int> allSelected;
  QList<int> allCandidate;

  QList<int> spkSceneSelected;
  QList<QPair<Episode *, QPair<qint64, qint64> > > summary;

  bool diff(false);

  // LSU(s) / scene
  QMap<int, QList<int> > sceneLsus = retrieveSceneLsus(sceneSpeechSegments, lsuSpeechSegments);
  QMap<int, int> lsuScene;

  // retrieve LSU features
  QList<qreal> lsuShotSize = getLsuShotSize(lsuShots, diff);
  QList<qreal> lsuMusicRate = getLsuMusicRate(lsuShots, diff);

  // retrieve interactions within LSUs
  m_socialNetProcessor->setSceneSpeechSegments(lsuSpeechSegments);
  m_socialNetProcessor->setGraph(SocialNetProcessor::Interact, false);
  QVector<QMap<QString, QMap<QString, qreal> > > lsuInteractions = m_socialNetProcessor->getSceneInteractions();

  // social dissimilarity between LSUs
  arma::mat SD = computeLsuSocialDistMat(lsuInteractions);

  // choose LSU(s) in current storyline sequence
  for (int i(0); i < sceneCovering.size(); i++) {

    QMap<QString, qreal> socialState = sceneCovering[i].first;
    QList<int> sceneIndices = sceneCovering[i].second;

    qDebug() << socialState;
    // qDebug() << sceneIndices;

    QList<int> lsus;
    QList<qreal> duration;
    QList<qreal> shotSize;
    QList<qreal> musicRate;
    QList<qreal> socialRelevance;

    for (int j(0); j < sceneIndices.size(); j++) {

      QList<int> currSceneLsus = sceneLsus[sceneIndices[j]];
      lsus.append(currSceneLsus);

      allCandidate.append(currSceneLsus);

      // qDebug() << sceneIndices[j] << currSceneLsus;

      for (int k(0); k < currSceneLsus.size(); k++) {

	qreal lsuSocialRelevance = getLsuSocialRelevance(lsuInteractions[currSceneLsus[k]], speaker, socialState);

	duration.push_back(lsuDuration[currSceneLsus[k]]);
	shotSize.push_back(lsuShotSize[currSceneLsus[k]]);
	musicRate.push_back(lsuMusicRate[currSceneLsus[k]]);
	socialRelevance.push_back(lsuSocialRelevance);
	
	// displayLsuContent(lsuShots[currSceneLsus[k]], lsuSpeechSegments[currSceneLsus[k]]);
	lsuScene[currSceneLsus[k]] = sceneIndices[j];
      }
    }
      
    // parameters of LSUs selection
    arma::vec P = computeCosts(shotSize, musicRate, socialRelevance, {styleWeight, styleWeight, 1.0});
    arma::vec W = computeWeights(duration);
    
    arma::uvec lsuIndices(lsus.size());
    for (int j(0); j < lsus.size(); j++)
      lsuIndices(j) = lsus[j];
    
    arma::mat CFS = FS.submat(lsuIndices, lsuIndices);
    arma::mat CSD = SD.submat(lsuIndices, lsuIndices);

    // qDebug() << lsus;
    QList<int> selected;
    qreal lambda(0.05);
    // qreal optValue = m_optimizer->knapsack(P, W, CFS, lambda * CSD, dur, selected);
    selected.clear();
    qreal heurValue = m_optimizer->mmr(P, W, CFS, lambda * CSD, dur, selected);
    // qDebug() << endl << "***" << optValue << heurValue << (heurValue / optValue) * 100.0 << "***" << endl;
    // qDebug() << selected;


    for (int j(0); j < selected.size(); j++) {

      allSelected.push_back(lsus[selected[j]]);

      qDebug() << lsus[selected[j]] << P(selected[j]) << W(selected[j]) << ":" << shotSize[selected[j]] << musicRate[selected[j]] << socialRelevance[selected[j]] << endl;
      displayLsuContent(lsuShots[lsus[selected[j]]], lsuSpeechSegments[lsus[selected[j]]]);
      qDebug() << endl << "\t" << lsuInteractions[lsus[selected[j]]];
      qDebug();

      // segment boundaries
      QPair<qint64, qint64> boundaries = getLsuBound(lsus[selected[j]], lsuShots, lsuSpeechSegments);
	
      // current episode
      Shot *first = lsuShots[lsus[selected[j]]].first();
      Episode *episode = dynamic_cast<Episode *>(first->parent()->parent());

      // possibly merge current and previous segments if consecutive
      if (!summary.isEmpty()) {
	Episode *prevEpisode = summary.last().first;
	QPair<qint64, qint64> prevBound = summary.last().second;
	if (episode == prevEpisode && boundaries.first <= prevBound.second) {
	  summary.pop_back();
	  boundaries.first = prevBound.first;
	}
      }
      
      summary.push_back(QPair<Episode *, QPair<qint64, qint64> >(episode, boundaries));
      steps.push_back(i);

      // write out scene of selected LSUs
      int lsuIdx = lsus[selected[j]];
      int lsuSceneIdx = lsuScene[lsuIdx];
      int spkSceneIdx = sceneMapping[lsuSceneIdx];
      qDebug() << lsuIdx << lsuSceneIdx << spkSceneIdx;
      spkSceneSelected.push_back(spkSceneIdx);
    }
  }

  arma::uvec S;
  S.zeros(spkSceneSelected.size());
  for (int i(0); i < spkSceneSelected.size(); i++)
    S(i) = spkSceneSelected[i];

  // QString dir = "/home/xavier/Dropbox/tv_series_proc_tool/tools/sna/matlab/data/";
  // QString fName = dir + speaker.toUpper().replace(" ", "_") + "_SF.dat";

  // S.save(fName.toStdString(), arma::raw_ascii);

  qDebug() << "# candidate LSUs:" << allCandidate.size();
  qDebug() << "avg. dur.:" << getAvgDuration(allCandidate, lsuDuration) << "s.";
  qDebug() << "# selected LSUs:" << allSelected.size();
  qDebug() << "avg. dur.:" << getAvgDuration(allSelected, lsuDuration) << "s.";

  return summary;
}

qreal MovieAnalyzer::getAvgDuration(const QList<int> &lsus, const QList<qreal> &lsuDuration)
{
  qreal dur(0.0);

  for (int i(0); i < lsus.size(); i++)
    dur += lsuDuration[lsus[i]];

  return dur / lsus.size();
}

void MovieAnalyzer::filterLsus(const QString &speaker, QList<QList<Shot *> > &lsuShots, QList<QList<SpeechSegment *> > &lsuSpeechSegments)
{
  for (int i(lsuSpeechSegments.size() - 1); i >= 0 ; i--) {

    int  j(0);
    bool found(false);
    while (j < lsuSpeechSegments[i].size() && !found) {
      QString currSpeaker = lsuSpeechSegments[i][j]->getLabel(Segment::Manual);
      if (currSpeaker == speaker)
	found = true;
      j++;
    }

    if (!found) {
      lsuShots.removeAt(i);
      lsuSpeechSegments.removeAt(i);
    }
  }
}

arma::mat MovieAnalyzer::computeLsuFormalSimMat(QList<QList<Shot *> > lsuShots)
{
  arma::mat S;
  S.eye(lsuShots.size(), lsuShots.size());

  for (int i(0); i < lsuShots.size() - 1; i++)
    for (int j(i+1); j < lsuShots.size(); j++) {
      qreal sim = computeJaccardSim(lsuShots[i], lsuShots[j]);
      S(i, j) = sim;
      S(j, i) = sim;
    }

  return S;
}

qreal MovieAnalyzer::computeJaccardSim(QList<Shot *> L1, QList<Shot *> L2)
{
  qreal sim(0.0);
  int nInter(0);

  if (L1.size() < L2.size()) {
    for (int i(0); i < L1.size(); i++)
      if (L2.contains(L1[i]))
	nInter++;
  }
  else {
    for (int i(0); i < L2.size(); i++)
      if (L1.contains(L2[i]))
	nInter++;
  }

  sim = static_cast<qreal>(nInter) / (L1.size() + L2.size() + nInter);

  return sim;
}

arma::mat MovieAnalyzer::computeLsuSocialSimMat(const QVector<QMap<QString, QMap<QString, qreal> > > &lsuInteractions)
{
  arma::mat S;
  S.eye(lsuInteractions.size(), lsuInteractions.size());

  for (int i(0); i < lsuInteractions.size() - 1; i++)
    for (int j(i+1); j < lsuInteractions.size(); j++) {
      S(i, j) = getLsuSocialSim(lsuInteractions[i], lsuInteractions[j]);
      S(j, i) = S(i, j);
    }

  return S;
}

qreal MovieAnalyzer::getLsuSocialSim(const QMap<QString, QMap<QString, qreal> > &lsu1, const QMap<QString, QMap<QString, qreal> > &lsu2)
{
  qreal scalProd(0.0);
  qreal l1(0.0);
  qreal l2(0.0);

  // compute first vector length
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = lsu1.begin();

  while (it1 != lsu1.end()) {

    QMap<QString, qreal> interlocs = it1.value();
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {

      qreal w = it2.value();
      l1 += qPow(w, 2);
      
      it2++;
    }

    it1++;
  }

  // compute scalar product and second vector length
  it1 = lsu2.begin();

  while (it1 != lsu2.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();
      
      scalProd += (w * lsu1[fSpeaker][sSpeaker]);
      l2 += qPow(w, 2);

      it2++;
    }

    it1++;
  }

  if (l1 == 0.0 || l2 == 0.0)
    return scalProd;

  return scalProd / (qSqrt(l1) * qSqrt(l2));
}

arma::mat MovieAnalyzer::computeLsuSocialDistMat(const QVector<QMap<QString, QMap<QString, qreal> > > &lsuInteractions)
{
  arma::mat D;
  D.zeros(lsuInteractions.size(), lsuInteractions.size());

  for (int i(0); i < lsuInteractions.size() - 1; i++)
    for (int j(i+1); j < lsuInteractions.size(); j++) {
      D(i, j) = getLsuSocialDist(lsuInteractions[i], lsuInteractions[j]);
      D(j, i) = D(i, j);
    }

  return D;
}

qreal MovieAnalyzer::getLsuSocialDist(const QMap<QString, QMap<QString, qreal> > &lsu1, const QMap<QString, QMap<QString, qreal> > &lsu2)
{
  qreal dist(0.0);
  qreal l1(0.0);
  qreal l2(0.0);

  // compute first vector length
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = lsu1.begin();

  while (it1 != lsu1.end()) {

    QMap<QString, qreal> interlocs = it1.value();
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {

      qreal w = it2.value();
      l1 += qPow(w, 2);
      
      it2++;
    }

    it1++;
  }

  // compute second vector length
  it1 = lsu2.begin();

  while (it1 != lsu2.end()) {

    QMap<QString, qreal> interlocs = it1.value();
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {

      qreal w = it2.value();
      l2 += qPow(w, 2);
      
      it2++;
    }

    it1++;
  }

  l1 = qSqrt(l1);
  l2 = qSqrt(l2);

  if (l1 == 0.0 || l2 == 0.0)
    return 0.0;

  // compute euclidean distance between LSUs
  it1 = lsu1.begin();

  while (it1 != lsu1.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();
      
      dist += qPow(w / l1 - lsu2[fSpeaker][sSpeaker] / l2, 2);

      it2++;
    }

    it1++;
  }

  it1 = lsu2.begin();

  while (it1 != lsu2.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();
      
      dist += qPow(w / l2 - lsu1[fSpeaker][sSpeaker] / l1, 2);

      it2++;
    }

    it1++;
  }

  return qSqrt(dist);
}

qreal MovieAnalyzer::getLsuSocialRelevance(const QMap<QString, QMap<QString, qreal> > &lsuInteractions, const QString &speaker, const QMap<QString, qreal> &socialState)
{
  qreal scalProd(0.0);
  qreal l1(0.0);
  qreal l2(0.0);

  // compute second vector length
  QMap<QString, qreal>::const_iterator it = socialState.begin();
  while (it != socialState.end()) {
    l2 += qPow(it.value(), 2);
    it++;
  }

  // commpute scalar product and first vector length
  QMap<QString, QMap<QString, qreal> >::const_iterator it1 = lsuInteractions.begin();

  while (it1 != lsuInteractions.end()) {

    QString fSpeaker = it1.key();
    QMap<QString, qreal> interlocs = it1.value();
    
    QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

    while (it2 != interlocs.end()) {
	
      QString sSpeaker = it2.key();
      qreal w = it2.value();

      if (fSpeaker == speaker) {
	scalProd += (w * socialState[sSpeaker]);
	l1 += qPow(w, 2);
      }

      else if (sSpeaker == speaker) {
	scalProd += w * socialState[fSpeaker];
	l1 += qPow(w, 2);
      }

      it2++;
    }

    it1++;
  }

  if (l1 == 0.0 || l2 == 0.0)
    return scalProd;

  return scalProd / (qSqrt(l1) * qSqrt(l2));
}

arma::vec MovieAnalyzer::computeCosts(const QList<qreal> &shotSize, const QList<qreal> &musicRate, const QList<qreal> &socialRelevance, QVector<qreal> lambda)
{
  arma::vec P;
  P.zeros(shotSize.size());

  // normalize coefficients of linear combination
  qreal sum(0.0);
  for (int i(0); i < lambda.size(); i++)
    sum += lambda[i];

  for (int i(0); i < lambda.size(); i++)
    lambda[i] /= sum;

  for (int i(0); i < shotSize.size(); i++)
    P(i) = lambda[0] * shotSize[i] + lambda[1] * musicRate[i] + lambda[2] * socialRelevance[i];

  return P;
}

arma::vec MovieAnalyzer::computeWeights(const QList<qreal> &duration)
{
  arma::vec W;
  W.zeros(duration.size());

  for (int i(0); i < duration.size(); i++)
    W(i) = duration[i];

  return W;
}

void MovieAnalyzer::displayLsuContent(QList<Shot *> lsuShots, QList<SpeechSegment *> lsuSpeechSegments)
{
  /*
  qDebug();

  for (int i(0); i < lsuShots.size(); i++)
    qDebug() << "\t" << lsuShots[i]->getPosition() / 1000.0 << lsuShots[i]->getEnd() / 1000.0 << (lsuShots[i]->getEnd() - lsuShots[i]->getPosition()) / 1000.0 << lsuShots[i]->getLabel(Segment::Automatic);
  */

  qDebug();

  for (int i(0); i < lsuSpeechSegments.size(); i++)
    qDebug() << "\t" << lsuSpeechSegments[i]->getPosition() / 1000.0 << lsuSpeechSegments[i]->getEnd() / 1000.0 << (lsuSpeechSegments[i]->getEnd() - lsuSpeechSegments[i]->getPosition()) / 1000.0 << lsuSpeechSegments[i]->getLabel(Segment::Manual);
}

QList<qreal> MovieAnalyzer::getLsuDuration(QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments)
{
  QList<qreal> lsuDuration;

  for (int i(0); i < lsuShots.size(); i++) {
    QPair<qint64, qint64> bound = getLsuBound(i, lsuShots, lsuSpeechSegments);
    lsuDuration.push_back((bound.second - bound.first) / 1000.0);
  }

  return lsuDuration;
}

QPair<qint64, qint64> MovieAnalyzer::getLsuBound(int i, QList<QList<Shot *> > lsuShots, QList<QList<SpeechSegment *> > lsuSpeechSegments)
 {
   qint64 start = lsuShots[i].first()->getPosition();
   qint64 end = lsuShots[i].last()->getEnd();

   qint64 speechSegmentStart(start);
   qint64 speechSegmentEnd(end);

   if (!lsuSpeechSegments[i].isEmpty()) {
     speechSegmentStart = lsuSpeechSegments[i].first()->getPosition();
     speechSegmentEnd = lsuSpeechSegments[i].last()->getEnd();
   }

   start = (start <= speechSegmentStart) ? start : speechSegmentStart;
   end = (end >= speechSegmentEnd) ? end - 40 : speechSegmentEnd;

   return QPair<qint64, qint64>(start, end);
 }

QList<qreal> MovieAnalyzer::getLsuShotSize(QList<QList<Shot *> > lsuShots, bool diff)
{
  QList<qreal> lsuShotSize;

  for (int i(0); i < lsuShots.size(); i++) {

    qreal currLsuShotSize(0.0);

    for (int j(0); j < lsuShots[i].size(); j++)
      
      if (diff && j > 0)
	currLsuShotSize += (lsuShots[i][j]->computeHeightRatio() - lsuShots[i][j-1]->computeHeightRatio());
      else if (!diff)
	currLsuShotSize += lsuShots[i][j]->computeHeightRatio();

    if (!diff)
      currLsuShotSize /= lsuShots[i].size();
    
    lsuShotSize.push_back(currLsuShotSize);
  }
  
  return lsuShotSize;
}

QList<qreal> MovieAnalyzer::getLsuMusicRate(QList<QList<Shot *> > lsuShots, bool diff)
{
  QList<qreal> lsuMusicRate;

  for (int i(0); i < lsuShots.size(); i++) {

    qreal currLsuMusicRate(0.0);
    QList<qreal> musicRates;

    for (int j(0); j < lsuShots[i].size(); j++) {

      QList<QPair<qint64, qreal> > shotMusicRates = lsuShots[i][j]->getMusicRates();

      for (int k(0); k < shotMusicRates.size(); k++)
	musicRates.push_back(shotMusicRates[k].second);
    }

    for (int j(0); j < musicRates.size(); j++)
      
      if (diff && j > 0)
	currLsuMusicRate += (musicRates[j] - musicRates[j-1]);
      else if (!diff)
	currLsuMusicRate += musicRates[j];

    if (!diff)
      currLsuMusicRate /= musicRates.size();

    lsuMusicRate.push_back(currLsuMusicRate);
  }
  
  return lsuMusicRate;
}

QMap<int, QList<int> > MovieAnalyzer::retrieveSceneLsus(QList<QList<SpeechSegment *> > sceneSpeechSegments, QList<QList<SpeechSegment *> > lsuSpeechSegments)
{
  QMap<int, QList<int> > sceneLsus;

  for (int i(0); i < lsuSpeechSegments.size(); i++)
    for (int j(0); j < sceneSpeechSegments.size(); j++)
      if (sceneSpeechSegments[j].contains(lsuSpeechSegments[i].first())) {
	sceneLsus[j].push_back(i);
	break;
      }
  
  return sceneLsus;
}

QList<QPair<QMap<QString, qreal>, QList<int> > > MovieAnalyzer::splitStoryLine(const QString &speaker, qreal granu, QList<QList<SpeechSegment *> > sceneSpeechSegments, QMap<int, int> &sceneMapping)
{
  QList<QPair<QMap<QString, qreal>, QList<int> > > sceneCovering;

  m_socialNetProcessor->setSceneSpeechSegments(sceneSpeechSegments);
  m_socialNetProcessor->setGraph(SocialNetProcessor::Interact, false);

  QList<QPair<int, QList<QPair<QString, qreal > > > > socialStates;

  arma::mat D = m_socialNetProcessor->getSocialStatesDist(speaker, socialStates);

  // mapping scenes -> scenes where speaker is involved
  for (int i(0); i < socialStates.size(); i++) {
    sceneMapping[socialStates[i].first] = i;
    qDebug() << i << socialStates[i].first;
  }

  // set covering
  QList<QList<int> > partition;
  QList<int> cIdx;
  arma::umat A;
  A.zeros(D.n_rows, D.n_cols);
  for (arma::uword i(0); i < D.n_rows; i++) {
    arma::uword j(i);

    /*
    for (arma::uword j(0); j < D.n_cols; j++)
      if (D(i, j) <= granu) {
	A(i, j) = 1;
	A(j, i) = A(i, j);
      }
    */

    while (j < D.n_cols && D(i, j) <= granu) {
      A(i, j) = 1;
      A(j, i) = A(i, j);
      j++;
    }
  }

  m_optimizer->setCovering(A, partition, cIdx);
  displayStoryLine(cIdx, socialStates, 5, false);

  // convert covering into partition
  for (int i(0); i < partition.size() - 1; i++)

    for (int j(partition[i].size() - 1); j >= 0; j--) {

      // distance to center in current subset
      qreal initD = D(cIdx[i], partition[i][j]);

      // looping over following subsets
      for (int k(i+1); k < partition.size(); k++) {
	  
	// instance exists in another subset
	if (partition[k].contains(partition[i][j])) {
	    
	  // distance to center in other subset
	  qreal currD = D(cIdx[k], partition[i][j]);
	    
	  // keep closest instance
	  if (currD <= initD && partition[i].contains(partition[i][j]))
	    partition[i].removeOne(partition[i][j]);
	  else
	    partition[k].removeOne(partition[i][j]);
	}
      }
    }

  qDebug() << partition;
  qDebug() << cIdx;
  qDebug() << partition.size() << "/" << socialStates.size() << ":" << (partition.size() / static_cast<qreal>(socialStates.size()));

  // QString dir = "/home/xavier/Dropbox/tv_series_proc_tool/tools/sna/matlab/data/";
  // QString fName = dir + speaker.toUpper().replace(" ", "_") + "_D.dat";

  // D.save(fName.toStdString(), arma::raw_ascii);

  arma::mat B = arma::conv_to<arma::mat>::from(A);
  arma::uvec C;
  C.zeros(cIdx.size());

  for (int i(0); i < cIdx.size(); i++) {

    arma::uword colIdx = static_cast<arma::uword>(cIdx[i]);
    
    C(i) = colIdx;

    arma::uvec IC = {colIdx};
    arma::uvec IR = arma::find(B.col(colIdx));
    
    arma::vec V(IR.size());
    V.fill(0.5);
    
    // B.submat(IR, IC) = V;
  }

  B = 1.0 - B;
  
  // fName = dir + speaker.toUpper().replace(" ", "_") + "_A.dat";
  // B.save(fName.toStdString(), arma::raw_ascii);

  // fName = dir + speaker.toUpper().replace(" ", "_") + "_C.dat";
  // C.save(fName.toStdString(), arma::raw_ascii);

  // write out boundaries of each part
  arma::umat P;
  P.zeros(partition.size(), 2); 

  for (int i(0); i < partition.size(); i++) {
    P(i, 0) = partition[i].first();
    P(i, 1) = partition[i].last();
  }

  // fName = dir + speaker.toUpper().replace(" ", "_") + "_P.dat";
  // P.save(fName.toStdString(), arma::raw_ascii);

  // populate scene covering with proper indices and vector of relationships
  for (int i(0); i < partition.size(); i++) {

    QMap<QString, qreal> relations;
    QList<int> subset;
    
    for (int j(0); j < partition[i].size(); j++) {

      if (cIdx.contains(partition[i][j])) {

	QList<QPair<QString, qreal> > interlocs = socialStates[partition[i][j]].second;

	for (int k(0); k < interlocs.size(); k++)
	  relations[interlocs[k].first] = interlocs[k].second;
      }

      subset.push_back(socialStates[partition[i][j]].first);

    }      

    sceneCovering.push_back(QPair<QMap<QString, qreal>, QList<int> >(relations, subset));
  }

  return sceneCovering;
}

void MovieAnalyzer::displayStoryLine(const QList<int> &cIdx, const QList<QPair<int, QList<QPair<QString, qreal > > > > &socialStates, int nbToDisp, bool all)
{
  for (int i(0); i < socialStates.size(); i++) {

    if (cIdx.contains(i))
      qDebug() << "****************";

    if (cIdx.contains(i) || all) {
      qDebug() << "Scene" << i << "->" << socialStates[i].first << endl;
    
      for (int j(0); j < socialStates[i].second.size(); j++)
	if (j < nbToDisp)
	  qDebug() << (socialStates[i].second)[j].first << (socialStates[i].second)[j].second;
    }
      
    if (cIdx.contains(i))
      qDebug() << "****************";
  }
}

qreal MovieAnalyzer::jaccardIndex(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const
{
  qreal jac(-1.0);

  arma::mat M = vectorize({hypInter, refInter}).first;
  arma::mat M_EW = M.col(0) % M.col(1);
  arma::uvec N_NULL = arma::find(M_EW);
  qreal cardInter = N_NULL.n_rows;
  jac = cardInter / M.n_rows;

  return jac;
}

qreal MovieAnalyzer::cosineSim(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const
{
  qreal cos(-1.0);
  
  arma::mat M = vectorize({hypInter, refInter}).first;
  cos = arma::norm_dot(M.col(0), M.col(1));

  return cos;
}

qreal MovieAnalyzer::l2Dist(const QMap<QString, QMap<QString, qreal> > &hypInter, const QMap<QString, QMap<QString, qreal> > &refInter) const
{
  qreal d(-1.0);

  arma::mat M = vectorize({hypInter, refInter}).first;
  M = arma::normalise(M, 2, 0);
  d = arma::norm(M.col(0) - M.col(1), 2);

  return d;
}

QPair<arma::mat, QList<QPair<QString, QString> > > MovieAnalyzer::vectorize(const QList<QMap<QString, QMap<QString, qreal> > > &inter, bool directed) const
{
  arma::mat M;
  QList<QPair<QString, QString> > mapping;

  // mapping
  for (int i(0); i < inter.size(); i++) {

    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = inter[i].begin();

    while (it1 != inter[i].end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interlocs = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

      while (it2 != interlocs.end()) {
      
	QString sSpeaker = it2.key();

	QPair<QString, QString> inter(fSpeaker, sSpeaker);
	
	if (!directed) {
	  inter.first = (fSpeaker <= sSpeaker) ? fSpeaker : sSpeaker;
	  inter.second = (fSpeaker > sSpeaker) ? fSpeaker : sSpeaker;
	}
	
	if (!mapping.contains(inter))
	  mapping.push_back(inter);

	it2++;
      }

      it1++;
    }
  }

  // populate matrix of vectorized interactions
  M.zeros(mapping.size(), inter.size());

  for (int j(0); j < inter.size(); j++) {

    QMap<QString, QMap<QString, qreal> >::const_iterator it1 = inter[j].begin();

    while (it1 != inter[j].end()) {

      QString fSpeaker = it1.key();
      QMap<QString, qreal> interlocs = it1.value();

      QMap<QString, qreal>::const_iterator it2 = interlocs.begin();

      while (it2 != interlocs.end()) {
      
	QString sSpeaker = it2.key();
	qreal weight = it2.value();

	QPair<QString, QString> inter(fSpeaker, sSpeaker);

	if (!directed) {
	  inter.first = (fSpeaker <= sSpeaker) ? fSpeaker : sSpeaker;
	  inter.second = (fSpeaker > sSpeaker) ? fSpeaker : sSpeaker;
	}

	int i = mapping.indexOf(inter);

	M(i, j) += weight;
	
	it2++;
      }

      it1++;
    }
  }

  return QPair<arma::mat, QList<QPair<QString, QString> > >(M, mapping);
}

bool MovieAnalyzer::extractScenes(QString fName, const QList<QString> &shotLabels, const QList<qint64> &shotPositions, const QList<QString> &utterLabels, const QList<QPair<qint64, qint64> > &utterBound)
{
  /**********************************************/
  /* retrieving image-based Logical Story Units */
  /**********************************************/

  // LSU boundaries
  QList<QPair<int, int> > lsuBound;

  /*
  // retrieving similarity matrix between shots
  arma::umat Y = computeSimShotMatrix(shotLabels, false);

  // retrieving image-based Logical Story Units (LSU)
  lsuBound = extractLSUs(Y, false, 3);
  */

  return true;
}

void MovieAnalyzer::getSpkDiarData(QList<SpeechSegment *> speechSegments)
{
  arma::mat X;
  arma::mat Sigma;
  arma::mat W;

  m_audioProcessor->extractIVectors(speechSegments);

  X = m_audioProcessor->getEpisodeIVectors(speechSegments);
  Sigma = m_audioProcessor->genSigmaMat();
  W = m_audioProcessor->genWMat();

  emit setSpkDiarData(X, Sigma, W);
}

QList<QList<Shot *> > MovieAnalyzer::retrieveLsuShots(QList<Shot *> shots, Segment::Source source, qint64 minDur, qint64 maxDur, bool rec, bool sceneBound)
{
  QList<QList<Shot *> > lsuShots;
  QList<QPair<int, int> > lsuBound;

  // retrieving similarity matrix between shots
  arma::umat Y = computeSimShotMatrix(shots, source, sceneBound);

  // retrieving boundaries of image-based Logical Story Units (LSU)
  lsuBound = extractLSUs(Y, rec, minDur, maxDur, shots);
  std::sort(lsuBound.begin(), lsuBound.end());
  
  // looping over LSU boundaries
  for (int i(0); i < lsuBound.size(); i++) {
    int nShots = lsuBound[i].second - lsuBound[i].first + 1;
    lsuShots.push_back(shots.mid(lsuBound[i].first, nShots));
  }

  return lsuShots;
}

bool MovieAnalyzer::coClustering(const QString &fName, QList<Shot *> shots, QList<QList<Shot *> > lsuShots, QList<SpeechSegment *> speechSegments, QList<QList<SpeechSegment *> > lsuSpeechSegments)
{
  arma::mat X;
  arma::mat Sigma;
  // arma::mat W;

  m_audioProcessor->extractIVectors(speechSegments);
  X = m_audioProcessor->getEpisodeIVectors(speechSegments);
  Sigma = m_audioProcessor->genSigmaMat();
  // W = m_audioProcessor->genWMat();

  // looping over LSUs
  for (int i(0); i < lsuShots.size(); i++) {
    // for (int i(26); i < 27; i++) {
    
    int n(lsuShots[i].size());
    int m(lsuSpeechSegments[i].size());

    // LSU contains at least two speech segments
    if (m > 1 && n * m < 1000) {

      // LSU shot/speech boundaries
      int shotLBound = shots.indexOf(lsuShots[i].first());
      int shotUBound = shots.indexOf(lsuShots[i].last());
      int utterLBound = speechSegments.indexOf(lsuSpeechSegments[i].first());
      int utterUBound = speechSegments.indexOf(lsuSpeechSegments[i].last());
      qDebug() << "LSU" << i << ":" << shotLBound << "->" << shotUBound << "/" << utterLBound << "->" << utterUBound;

      // computing matrix of correlation between shots
      int n(lsuShots[i].size());
      arma::mat DS(n, n, arma::fill::zeros);
      setShotCorrMatrix(DS, fName, lsuShots[i], 5, 6);

      // computing matrix of distances between speech segments
      arma::mat DU = retrieveUtterMatDist(X, Sigma, UtteranceTree::L2, lsuSpeechSegments[i], speechSegments);

      // computing temporal distribution of shots over utterances
      QList<QList<SpeechSegment *> > shotSpeechSegments = retrieveShotSpeechSegments(lsuShots[i], lsuSpeechSegments[i], true);
      arma::mat A = computeOverlapShotUtterMat(lsuShots[i], lsuSpeechSegments[i], shotSpeechSegments);

      /*
      cout << DS << endl;
      cout << DU << endl;
      cout << A << endl;
      */

      // retrieving number of reference shot clusters/speakers
      int p = getNbRefShotClusters(lsuShots[i]);
      int pp = getNbRefSpeakers(lsuSpeechSegments[i]);

      // shot and speech segments clusters/centers
      QList<QList<int> > shotPartition;
      QList<int> shotCIdx;
      QList<QList<int> > utterPartition;
      QList<int> utterCIdx;
      QMap<int, QList<int> > mapping;
      qreal lambda(0.5);

      m_optimizer->coClusterOptMatch_iter(p, DS, shotPartition, shotCIdx, A, pp, DU, utterPartition, utterCIdx, mapping, lambda);

      // displaying shot clusters
      for (int j(0); j < shotPartition.size(); j++) {
	qDebug() << "Center" << shots.indexOf(lsuShots[i][shotCIdx[j]]) << lsuShots[i][shotCIdx[j]]->getLabel(Segment::Manual);
	for (int k(0); k < shotPartition[j].size(); k++) {
	  qDebug() << shots.indexOf(lsuShots[i][shotPartition[j][k]]) << lsuShots[i][shotPartition[j][k]]->getLabel(Segment::Manual);
	}
	qDebug();
      }

      // displaying utterance clusters
      for (int j(0); j < utterPartition.size(); j++) {
	qDebug() << "Center" << speechSegments.indexOf(lsuSpeechSegments[i][utterCIdx[j]]) << lsuSpeechSegments[i][utterCIdx[j]]->getLabel(Segment::Manual);
	for (int k(0); k < utterPartition[j].size(); k++) {
	  qDebug() << speechSegments.indexOf(lsuSpeechSegments[i][utterPartition[j][k]]) << lsuSpeechSegments[i][utterPartition[j][k]]->getLabel(Segment::Manual);
	}
	qDebug();
      }

      // matching speakers <-> shot clusters
      qDebug() << "Optimal matching:";
      QMap<int, QList<int> >::const_iterator it =  mapping.begin();
      while (it != mapping.end()) {
	int uttIdx = speechSegments.indexOf(lsuSpeechSegments[i][it.key()]);
	QList<int> shotClustIdx = it.value();
	
	for (int j(0); j < shotClustIdx.size(); j++) {
	  int shotIdx = shots.indexOf(lsuShots[i][shotClustIdx[j]]);
	  qDebug() << uttIdx << "<->" << shotIdx;
	}

	qDebug();

	it++;
      }
    }
  }

  return true;
}

int MovieAnalyzer::getNbRefShotClusters(QList<Shot *> shots)
{
  QList<QString> refLabels;

  for (int i(0); i < shots.size(); i++) {
    QString label = shots[i]->getLabel(Segment::Manual);
    if (!refLabels.contains(label))
      refLabels.push_back(label);
  }

  return refLabels.size();
}

int MovieAnalyzer::getNbRefSpeakers(QList<SpeechSegment *> speechSegments)
{
  QList<QString> refLabels;

  for (int i(0); i < speechSegments.size(); i++) {
    QString label = speechSegments[i]->getLabel(Segment::Manual);
    if (!refLabels.contains(label))
      refLabels.push_back(label);
  }

  return refLabels.size();
}

QPair<int, int> MovieAnalyzer::getLSUSpeechBound(QList<QList<QPair<int, qreal> > > shotUtterances, int firstShot, int lastShot)
{
  QPair<int, int> bound;

  // retrieving index of first utterance
  int i(firstShot);
  while (i <= lastShot && shotUtterances[i].isEmpty())
    i++;

  // LSU does not contain speech
  if (i > lastShot)
    return QPair<int, int>(-1, -1);

  bound.first = shotUtterances[i][0].first;

  // retrieving index of last utterance
  i = lastShot;
  while (i >= firstShot && shotUtterances[i].isEmpty())
    i--;

  bound.second = shotUtterances[i][shotUtterances[i].size()-1].first;

  return bound;
}

QList<QList<QPair<int, qreal> > > MovieAnalyzer::retrieveShotUtterances(const QList<qint64> &shotPositions, const QList<QPair<qint64, qint64> > &utterBound, bool includeAll)
{
  QList<QList<QPair<int, qreal> > > shotUtterances;

  int nShots(shotPositions.size());
  int j(0);

  for (int i(0); i < nShots; i++) {

    // utterances covered by current shot
    QList<QPair<int, qreal> > currShotUtterances;

    // shot boundaries in milliseconds
    qint64 shotStart(shotPositions[i]);
    qint64 shotEnd(shotPositions[i+1]);

    // indices of first and last utterances of current shot
    int idxFirstUtter;
    int idxLastUtter;

    // retrieve all utterances within current shot, included
    // overlapping ones
    if (includeAll) {
      
      // looping over utterances untill current shot is reached
      while (j < utterBound.size() && utterBound[j].second < shotStart)
	j++;

      idxFirstUtter = j;
    
      // looping over utterances untill current shot is exited
      while (j < utterBound.size() && utterBound[j].first < shotEnd)
	j++;

      idxLastUtter = --j;
    }

    // retrieve utterances whose major part is covered within current
    // shot
    else {
      
      // looping over utterances untill current shot is reached
      while (j < utterBound.size() && (utterBound[j].second - shotStart) < (utterBound[j].second - utterBound[j].first) / 2.0)
	j++;

      idxFirstUtter = j;
    
      // looping over utterances untill current shot is exited
      while (j < utterBound.size() && (utterBound[j].second - utterBound[j].first) / 2.0 < (shotEnd - utterBound[j].first))
	j++;

      idxLastUtter = --j;
    }

    // current shot does not contain any speech segment
    if (idxLastUtter < idxFirstUtter)
      idxFirstUtter = idxLastUtter = -1;
    
    if (j == -1)
      j = 0;

    // push covered utterance to current list
    if (idxFirstUtter != -1) {

      for (int k(idxFirstUtter); k <= idxLastUtter; k++) {
	qint64 utterStart = (utterBound[k].first < shotStart) ? shotStart : utterBound[k].first;
	qint64 utterEnd = (utterBound[k].second > shotEnd) ? shotEnd : utterBound[k].second;
	qreal utterDur = (utterEnd - utterStart) / 1000.0;
	currShotUtterances.push_back(QPair<int, qreal>(k, utterDur));
      }
    }

    // add list of shots
    shotUtterances.push_back(currShotUtterances);
  }

  return shotUtterances;
}

QList<QList<SpeechSegment *> > MovieAnalyzer::retrieveShotSpeechSegments(QList<Shot *> shots, QList<SpeechSegment *> speechSegments, bool includeAll)
{
  QList<QList<SpeechSegment *> > shotSpeechSegments;

  int n(shots.size());
  int j(0);

  for (int i(0); i < n; i++) {

    // speech segments covered by current shot
    QList<SpeechSegment *> currShotSpeechSegments;

    // shot boundaries in milliseconds
    qint64 shotStart(shots[i]->getPosition());
    qint64 shotEnd(shots[i]->getEnd());

    // indices of first and last utterances of current shot
    int idxFirstSeg;
    int idxLastSeg;

    // retrieve all utterances within current shot, included
    // overlapping ones
    if (includeAll) {
      
      // looping over utterances untill current shot is reached
      while (j < speechSegments.size() && speechSegments[j]->getEnd() <= shotStart)
	j++;

      idxFirstSeg = j;
    
      // looping over utterances untill current shot is exited
      while (j < speechSegments.size() && speechSegments[j]->getPosition() < shotEnd)
	j++;

      idxLastSeg = --j;
    }

    // retrieve utterances whose major part is covered within current
    // shot
    else {
      
      // looping over utterances untill current shot is reached
      while (j < speechSegments.size() && (speechSegments[j]->getEnd() - shotStart) < (speechSegments[j]->getEnd() - speechSegments[j]->getPosition()) / 2.0)
	j++;

      idxFirstSeg = j;
    
      // looping over utterances untill current shot is exited
      while (j < speechSegments.size() && (speechSegments[j]->getEnd() - speechSegments[j]->getPosition()) / 2.0 < (shotEnd - speechSegments[j]->getPosition()))
	j++;

      idxLastSeg = --j;
    }

    // push covered speech segments to current list
    if (idxFirstSeg <= idxLastSeg) {

      for (int k(idxFirstSeg); k <= idxLastSeg; k++)
	currShotSpeechSegments.push_back(speechSegments[k]);
    }

    // add list of speech segments
    shotSpeechSegments.push_back(currShotSpeechSegments);

    // reset j
    if (j == -1)
      j = 0;
  }

  return shotSpeechSegments;
}

arma::umat MovieAnalyzer::computeSimShotMatrix(QList<Shot *> shots, Segment::Source source, bool sceneBound)
{
  // number of shots
  int n = shots.size();

  arma::umat C;
  C.zeros(n, n);

  arma::umat Y;
  Y.eye(n, n);

  QRegularExpression label("C(\\d+)");

  for (int i(0); i < n; i++) {
   
    QString shotLabel = shots[i]->getLabel(source);

    QRegularExpressionMatch match = label.match(shotLabel);
    
    if (match.hasMatch()) {
      int j = match.captured(1).toInt();
      C(i, j) = 1;
    }
  }
    
  for (int j(0); j < n; j++) {
    
    arma::umat V = arma::find(C.col(j));

    if (V.n_rows > 0) {
      for (arma::uword i(0); i < V.n_rows; i++)
	for (arma::uword k(i+1); k < V.n_rows; k++) {
	  Y(V(i, 0), V(k, 0)) = 1;
	  Y(V(k, 0), V(i, 0)) = 1;
	}
    }
  }

  // disable shot similarity between different scenes
  if (sceneBound)
    for (int i(0); i < n; i++) {

      int shotIndex = shots[i]->row();

      if (shotIndex == 0 && i > 1) {
	arma::umat R;
	R.zeros(n - i, i);
	Y.submat(i, 0, n - 1, i - 1) = R;
	Y.submat(0, i, i - 1, n - 1) = R.t();
      }
    }

  return Y;
}

arma::umat MovieAnalyzer::computeSimUtterMatrix(const QList<QString> &utterLabels)
{
  // number of utterances
  int n = utterLabels.size();

  // list of utterance indices by reference speaker
  QMap<QString, QList<int> > spkUtter;
  
  // similarity matrix
  arma::umat Z;
  Z.eye(n, n);

  for (int i(0); i < n; i++)
    if (utterLabels[i] != "S")
      spkUtter[utterLabels[i]].push_back(i);

  QMap<QString, QList<int> >::const_iterator it = spkUtter.begin();

  while (it != spkUtter.end()) {

    QString speaker = it.key();
    QList<int> utter = it.value();

    for (int i(0); i < utter.size(); i++)
      for (int j(i+1); j < utter.size(); j++) {
	Z(utter[i], utter[j]) = 1;
	Z(utter[j], utter[i]) = 1;
      }

    it++;
  }

  return Z;
}

arma::mat MovieAnalyzer::computeOverlapShotUtterMat(QList<Shot *> lsuShots, QList<SpeechSegment *> lsuSpeechSegments, QList<QList<SpeechSegment *> > shotSpeechSegments)
{
  // matrix containing temporal distribution of shots over speech
  // segments
  arma::mat A(lsuShots.size(), lsuSpeechSegments.size(), arma::fill::zeros);

  // filling matrix
  for (int i(0); i < shotSpeechSegments.size(); i++) {
    for (int j(0); j < shotSpeechSegments[i].size(); j++) {

      qint64 start = (lsuShots[i]->getPosition() < shotSpeechSegments[i][j]->getPosition()) ? shotSpeechSegments[i][j]->getPosition() : lsuShots[i]->getPosition();
      qint64 end = (lsuShots[i]->getEnd() < shotSpeechSegments[i][j]->getEnd()) ? lsuShots[i]->getEnd() : shotSpeechSegments[i][j]->getEnd();

      qreal overlapTime = (end - start) / 1000.0;
      
      int n_row = i;
      int n_col = lsuSpeechSegments.indexOf(shotSpeechSegments[i][j]);
      A(n_row, n_col) = overlapTime;
    }
  }
  
  return A;
}

QList<QPair<int, int> > MovieAnalyzer::extractLSUs(const arma::umat &Y, bool rec, qint64 minDur, qint64 maxDur, QList<Shot *> shots)
{
  QList<QPair<int, int> > sceneBound;

  int inf(0);
  int sup(0);

  if (minDur == -1)
    extractLSUs_aux(inf, shots.size() - 1, Y, minDur, shots, sceneBound, rec);

  else
    while (inf < shots.size()) {

      while (sup < shots.size() && shots[sup]->getEnd() - shots[inf]->getPosition() <= maxDur)
	sup++;

      // consider sequences containing at least 3 shots
      if (sup - inf >= 2)
	extractLSUs_aux(inf, sup - 1, Y, minDur, shots, sceneBound, rec);

      inf++;
    }
  
  return sceneBound;
}

void MovieAnalyzer::extractLSUs_aux(int first, int last, const arma::umat &Y, qint64 minDur, QList<Shot *> shots, QList<QPair<int, int> > &sceneBound, bool rec)
{
  qint64 duration = shots[last]->getEnd() - shots[first]->getPosition();

  if (duration < minDur)
    return;

  // number of shots in current sequence
  int n(last-first+1);

  // initialize column and row sum vectors
  arma::uvec C(n-1);
  arma::uvec R(n-1);
  int j(0);
  for (int i(first); i < last; i++) {

    arma::uvec T = Y.submat(i+1, i, last, i);
    C(j) = sum(T);

    T = (Y.submat(last-j, first, last-j, last-j-1)).t();
    R(n-2-j) = sum(T);

    j++;
  }

  // search for LSUs
  int nSurround(0);
  int prev(0);
  int start(first);
  int end(last);

  j = 1;
  for (int i(first+1); i < last; i++) {

    nSurround += (C(j-1) - R(j-1));

    // beginning of new LSU
    if (nSurround > 0 && prev == 0)
      start = i - 1;

    // end of previous LSU
    else if (nSurround == 0 && prev > 0) {
      
      end = i;
      QPair<int, int> bound(start, end);
      qint64 duration = shots[end]->getEnd() - shots[start]->getPosition();

      if (duration >= minDur && !sceneBound.contains(bound)) {
	
	sceneBound.push_back(bound);

	if (rec) {
	  extractLSUs_aux(start + 1, end, Y, minDur, shots, sceneBound, rec);
	  extractLSUs_aux(start, end - 1, Y, minDur, shots, sceneBound, rec);
	}
      }
    }

    prev = nSurround;
    j++;
  }

  // possibly add last LSU
  if (prev > 0) {

    end = last;
    QPair<int, int> bound(start, end);
    qint64 duration = shots[end]->getEnd() - shots[start]->getPosition();

    if (duration >= minDur && !sceneBound.contains(bound)) {

      sceneBound.push_back(bound);

      if (rec) {
	extractLSUs_aux(start + 1, end, Y, minDur, shots, sceneBound, rec);
	extractLSUs_aux(start, end - 1, Y, minDur, shots, sceneBound, rec);
      }
    }
  }
}

qreal MovieAnalyzer::meanDistance(const QVector<qreal> &distance)
{
  qreal sum(0.0);

  for (int i(0); i < distance.size(); i++)
    sum += abs(1 - distance[i]);

  return 1 - sum / distance.size();
}



///////////////////////
// auxiliary methods //
///////////////////////

void MovieAnalyzer::normalize(arma::mat &E, const arma::mat &CovInv, UtteranceTree::DistType dist)
{
  qreal normFac;

  for (arma::uword i(0); i < E.n_rows; i++) {

    switch (dist) {

    case UtteranceTree::L2:
      normFac = arma::as_scalar(E.row(i) * E.row(i).t());
      break;

    case UtteranceTree::Mahal:
      normFac = arma::as_scalar(E.row(i) * CovInv * E.row(i).t());
      break;
    }

    E.row(i) = E.row(i) / normFac;
  }
}


///////////
// slots //
///////////

void MovieAnalyzer::setSpeakerPartition(QList<QList<int> > partition)
{
  QString speaker;
  QList<QPair<qreal, qreal> > utterances;
  QList<QPair<int, qreal> > utter;
  QList<QList<int> > newPartition;

  // looping over the partition elements
  for (int i(0); i < partition.size(); i++) {
    QList<int> part = partition[i];
    QList<int> newPart;

    // looping over each part
    for (int j(0); j < part.size(); j++) {
      speaker = m_speakers[part[j]];
      utterances = m_utterances[speaker];

      // looping over the utterances of each speaker for labelling purpose
      for (int k(0); k < utterances.size(); k++) {

	QPair<qint64, qint64> bound = QPair<qint64, qint64>(static_cast<qint64>(utterances[k].first * 1000), static_cast<qint64>(utterances[k].second * 1000));

	int uttIdx = m_subBound.indexOf(bound);
	
	newPart.push_back(uttIdx);

	emit setSpeaker(static_cast<qint64>(utterances[k].first * 1000),
			static_cast<qint64>(utterances[k].second * 1000),
			"S" + QString::number(i),
			VideoFrame::Hyp1);
      }
    }

    newPartition.push_back(newPart);
  }

  for (int i(0); i < m_subBound.size(); i++) {
    qreal duration = m_subBound[i].second / 1000.0 - m_subBound[i].first / 1000.0;
    utter.push_back(QPair<int, qreal>(i, duration));
  }

  // QPair<qreal, qreal> res(computeSpkError(newPartition, utter, m_subBound, m_refLbl));

  emit setLocalDer(m_localDer);
  // emit setGlobalDer(QString::number(res.first, 'g', 4));
}

void MovieAnalyzer::playSpeakers(QList<int> speakers)
{
  QList<QPair<qreal, qreal>> utterances;
  QList<QPair<qint64, qint64>> segments;
  QString speaker;

  for (int i(0); i < speakers.size(); i++) {
    speaker = m_speakers[speakers[i]];
    utterances = m_utterances[speaker];
     
    for (int j(0); j < utterances.size(); j++)
      segments.push_back(QPair<qint64, qint64>(utterances[j].first * 1000, utterances[j].second * 1000));
  }
  
  // emit playSegments(segments);
}

qreal MovieAnalyzer::retrieveDer(QString fName)
{
  QRegularExpression score("OVERALL SPEAKER DIARIZATION ERROR = (\\d+.\\d{2})");
  QFile file(fName);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return -2.0;

  QTextStream in(&file);

  // parsing score file
  while (!in.atEnd()) {
    QString line = in.readLine();
    QRegularExpressionMatch match = score.match(line);
    
    if (match.hasMatch())
      return match.captured(1).toDouble();
  }

  return -1.0;
}

arma::mat MovieAnalyzer::computeDistMat(const arma::mat &S, const arma::mat &SigmaInv, UtteranceTree::DistType dist)
{
  arma::mat D(S.n_rows, S.n_rows, arma::fill::zeros);
  qreal d;

  for (arma::uword i(0); i < S.n_rows; i++)
    for (arma::uword j(i); j < S.n_rows; j++) {
      d = computeDistance(S.row(i), S.row(j), SigmaInv, dist);
      D(i, j) = d;
      D(j, i) = d;
    }
  
  return D;
}

qreal MovieAnalyzer::computeDistance(const arma::mat &U, const arma::mat &V, const arma::mat &SigmaInv, UtteranceTree::DistType dist)
{
  qreal d(0.0);
  arma::mat diff;

  switch (dist) {
  case UtteranceTree::L2:
    d = norm(U - V);
    break;
  case UtteranceTree::Mahal:
    diff = U - V;
    d = as_scalar(sqrt(diff * SigmaInv * diff.t()));
    break;
  }

  return d;
}

arma::mat MovieAnalyzer::retrieveUtterMatDist(arma::mat X, arma::mat CovInv, UtteranceTree::DistType dist, QList<SpeechSegment *> lsuSpeechSegments, QList<SpeechSegment *> speechSegments)
{
  // number of instances
  int m(lsuSpeechSegments.size());

  // indices of utterances in current pattern
  arma::umat V(1, m);

  for (int i(0); i < m; i++)
    V(0, i) = speechSegments.indexOf(lsuSpeechSegments[i]);

  // matrix containing utterance i-vectors for current pattern
  arma::mat S = X.rows(V);

  // computing distance matrix
  return computeDistMat(S, CovInv, dist);
}

QPair<qreal, qreal> MovieAnalyzer::computeSpkError(QList<QList<int> > &partition, QList<SpeechSegment *> speechSegments, QString pattLabel)
{
  QList<QString> refList;                    // reference speakers
  QList<QString> hypList;                    // hypothesized speakers
  QMap<QString, QString> optMap;             // optimal mapping between reference and hyppothesized speakers
  qreal duration(0.0);                       // pattern duration
  qreal errTime(0.0);                        // speaker error duration
  QPair<qreal, qreal> res(-1.0, -1.0);       // values to return
  QMap<QString, QMap<QString, qreal> > dist; // distribution of reference speakers over hypothesized ones

  // looping over partition for labelling purpose
  for (int i(0); i < partition.size(); i++) {
    for (int j(0); j < partition[i].size(); j++) {
      
      // index of current utterance
      int uttIdx = partition[i][j];

      // reference speaker
      QString refSpk = speechSegments[uttIdx]->getLabel(Segment::Manual);

      // hypothesized speaker
      QString spkPrefix = "";
      if (pattLabel != "")
	spkPrefix = pattLabel + "_";
      QString hypSpk = spkPrefix + "S" + QString::number(i);

      // utterance boundaries
      qreal uttStart = speechSegments[uttIdx]->getPosition() / 1000.0;
      qreal uttEnd = speechSegments[uttIdx]->getEnd() / 1000.0;
      qreal uttDur = uttEnd - uttStart;

      // updating lists of speakers
      if (!refList.contains(refSpk))
	refList.push_back(refSpk);
      if (!hypList.contains(hypSpk))
	hypList.push_back(hypSpk);
      
      // updating ditribution of hypothesized speakers over reference ones
      dist[refSpk][hypSpk] += uttDur;

      speechSegments[uttIdx]->setLabel(hypSpk, Segment::Automatic);
    }
  }

  // retrieve optimal mapping between reference and hypothesized partitions
  optMap = m_optimizer->optimalMatching(refList, hypList, dist);

  for (int i(0); i < partition.size(); i++) {
    for (int j(0); j < partition[i].size(); j++) {

      // index of current utterance
      int uttIdx = partition[i][j];

      // reference speaker
      QString refSpk = speechSegments[uttIdx]->getLabel(Segment::Manual);

      // hypothesized speaker
      QString hypSpk = speechSegments[uttIdx]->getLabel(Segment::Automatic);

      // utterance boundaries
      qreal uttStart = speechSegments[uttIdx]->getPosition() / 1000.0;
      qreal uttEnd = speechSegments[uttIdx]->getEnd() / 1000.0;
      qreal uttDur = uttEnd - uttStart;

      // updating amount of speech within pattern
      duration += uttDur;

      // update speaker error time
      if (optMap[refSpk] != hypSpk)
	errTime += uttDur;
    }
  }

  res.first = errTime / duration * 100;
  res.second = duration;

  return res;
}

 qreal MovieAnalyzer::retrieveSSDer(QList<QPair<qreal, qreal> > &localDer)
{
  qreal totDuration(0.0);
  qreal ssDer(0.0);

  for (int i(0); i < localDer.size(); i++) {
    ssDer += localDer[i].first * localDer[i].second;
    totDuration += localDer[i].second;
  }

  ssDer /= totDuration;

  return ssDer;
}
