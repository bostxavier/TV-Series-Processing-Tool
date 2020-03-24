#include "AudioProcessor.h"

#include <QRegularExpression>
#include <QMessageBox>
#include <QMediaPlayer>

#include "Episode.h"

using namespace arma;

AudioProcessor::AudioProcessor(QWidget *parent)
  : QWidget(parent)
{
  m_paramProcess = new QProcess;
  m_normProcess = new QProcess;
  m_tvProcess = new QProcess;
  m_ivProcess = new QProcess;
  m_genXProcess = new QProcess;
  m_cleanDirProcess = new QProcess;

  m_convertProcess = new QProcess;
  m_musicTrackingProcess = new QProcess;

  m_output = new QLabel;
  m_output->setFixedSize(450, 75);
  m_output->setWindowTitle(tr("Extracting i-vectors..."));
  m_output->hide();

  connect(m_paramProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(normalizeParameters(int, QProcess::ExitStatus)));
  connect(m_normProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(estimateTVMatrix(int, QProcess::ExitStatus)));
  connect(m_tvProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(genIVectors(int, QProcess::ExitStatus)));
  connect(m_ivProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(genXMatrix(int, QProcess::ExitStatus)));
  connect(m_genXProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(cleanDir(int, QProcess::ExitStatus)));
  connect(m_cleanDirProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ivExtractionCompleted(int, QProcess::ExitStatus)));

  connect(m_convertProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(extractMusicRate(int, QProcess::ExitStatus)));
  connect(m_musicTrackingProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(retrieveMusicRate(int, QProcess::ExitStatus)));

  connect(m_paramProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(parameterizeOutput()));
  connect(m_normProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(normalizeOutput()));
  connect(m_tvProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(tvEstimateOutput()));
  connect(m_ivProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(genIVectorsOutput()));

  connect(m_convertProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(convertOutput()));
  connect(m_musicTrackingProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(musicTrackingOutput()));
}

void AudioProcessor::extractIVectors(QList<SpeechSegment *> speechSegments)
{
  // m_output->show();

  if (extractAudioFiles(speechSegments))
    parameterizeSegments();
}

arma::mat AudioProcessor::getEpisodeIVectors(QList<SpeechSegment *> speechSegments)
{
  arma::mat X;

  // retrieve current episode features
  Segment *episode = speechSegments[0]->parent();
  int seasNbr = episode->parent()->getNumber();
  int epNbr = episode->getNumber();

  if (m_epBound.contains(seasNbr) && m_epBound[seasNbr].contains(epNbr)) {
   
    int firstRow = m_epBound[seasNbr][epNbr].first;
    int lastRow = m_epBound[seasNbr][epNbr].second;
    
    if (firstRow < static_cast<int>(m_X.n_rows) && lastRow < static_cast<int>(m_X.n_rows))
      X = m_X.submat(firstRow, 0, lastRow, m_X.n_cols - 1);
  }
  
  return X;
}

arma::mat AudioProcessor::genWMat()
{
  // computing W
  mat W;

  if (m_spkIdx.size() > 0) {
    
    int nUtter(m_X.n_rows);
    umat spkRows = zeros<umat>(m_spkIdx.size(), nUtter);

    QMap<QString, QList<int> >::const_iterator it = m_spkIdx.begin();
  
    int i(0);

    while (it != m_spkIdx.end()) {

      QList<int> indices = it.value();

      for (int j(0); j < indices.size(); j++)
	if (indices[j] < static_cast<int>(m_X.n_rows))
	  spkRows(i, indices[j]) = 1;

      it++;
      i++;
    }

    int m(m_X.n_rows);
    int n(m_X.n_cols);
    int nSpk(spkRows.n_rows);
  
    W.zeros(n, n);

    // looping over the speakers
    for (int i(0); i < nSpk; i++) {
    
      // index of current speaker utterances in X matrix
      umat idx = arma::find(spkRows.row(i));
    
      // number of current speaker utterances
      int nUtt(idx.n_rows);

      // submatrix of vectorized speaker utterances
      mat S = m_X.rows(idx);

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
    W = cov(m_X);

  return W;
}

arma::mat AudioProcessor::genSigmaMat()
{
  return cov(m_X);
}

void AudioProcessor::trackMusic(const QString &epFName, int frameRate, int mtWindowSize, int mtHopSize, int chromaStaticFrameSize, int chromaDynamicFrameSize)
{
  extractAudioFile(epFName, frameRate);

  QString waveFile = "data/wav/audioFile.wav";
  m_musicTrackArgs.clear();
  m_musicTrackArgs << "musicTracking/RUN_music_tracker.sh" << waveFile << QString::number(frameRate) << QString::number(mtWindowSize) << QString::number(mtHopSize) << QString::number(chromaStaticFrameSize) << QString::number(chromaDynamicFrameSize);
}

void AudioProcessor::extractAudioFile(const QString &epFName, int frameRate)
{
  QString program;
  QStringList arguments;

  // converting current episode video file into audio one
  QString waveFile = "musicTracking/data/wav/audioFile.wav";
  
  qDebug() << "Extracting audio file...";

  program = "ffmpeg";
  arguments << "-i" << epFName << "-map" << "0:1" << "-vn" << "-acodec" << "pcm_s16le" <<"-ar" << QString::number(frameRate) << "-ac" << "2" << waveFile;
  m_convertProcess->start(program, arguments);
}


///////////
// slots //
///////////

void AudioProcessor::parameterizeSegments()
{
  QString program;
  QStringList arguments;
  
  m_output->setWindowTitle(tr("Extracting i-vectors: 2/5..."));

  // parameterizing speech segments
  program = "sh";
  arguments << "spkDiarization/01_RUN_feature_extraction.sh";
  m_paramProcess->start(program, arguments);
}

void AudioProcessor::normalizeParameters(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);

  QString program;
  QStringList arguments;

  if (exitStatus == QProcess::NormalExit) {

    m_output->setWindowTitle(tr("Extracting i-vectors: 3/5..."));

    // normalizing parameters
    program = "sh";
    arguments << "spkDiarization/02a_RUN_spro_front-end.sh";
    m_normProcess->start(program, arguments);
  }

  else
    QMessageBox::critical(this, tr("Parameterization"), tr("An Error occurred while parameterizing speech segments"));

}

void AudioProcessor::estimateTVMatrix(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);

  QString program;
  QStringList arguments;

  if (exitStatus == QProcess::NormalExit) {

    m_output->setWindowTitle(tr("Extracting i-vectors: 4/5..."));

    // estimating total variability matrix
    program = "sh";
    arguments << "spkDiarization/04_RUN_tv_estimation.sh";
    m_tvProcess->start(program, arguments);
  }
   else
     QMessageBox::critical(this, tr("Normalization"), tr("An Error occurred while normalizing parameters"));
}

void AudioProcessor::genIVectors(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);

  QString program;
  QStringList arguments;

  if (exitStatus == QProcess::NormalExit) {

    m_output->setWindowTitle(tr("Extracting i-vectors: 5/5..."));

    // extracting i-vectors
    program = "sh";
    arguments << "spkDiarization/05_RUN_i-vector_extraction.sh";
    m_ivProcess->start(program, arguments);
  }
  else
    QMessageBox::critical(this, tr("Total Variability matrix estimation"), tr("An Error occurred while estimating TV Matrix"));
}
 
void AudioProcessor::genXMatrix(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);

  QString program;
  QStringList arguments;

  if (exitStatus == QProcess::NormalExit) {

    // exporting i-vectors into readable matrix
    program = "sh";
    arguments << "spkDiarization/06_RUN_X_mat_generate.sh" << m_seriesName;
    m_genXProcess->start(program, arguments);
  }
  else
    QMessageBox::critical(this, tr("I-vectors extraction"), tr("An Error occurred while extracting i-vectors"));

  QString ivFName = "spkDiarization/iv/X_" + m_seriesName + ".dat";
  m_X.load(ivFName.toStdString());
}

void AudioProcessor::cleanDir(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);
  Q_UNUSED(exitStatus);

  QString program;
  QStringList arguments;

  if (exitStatus == QProcess::NormalExit) {

    // cleaning work directories
    program = "sh";
    arguments << "spkDiarization/00_RUN_clean_directories.sh";
    m_cleanDirProcess->start(program, arguments);
  }
}

void AudioProcessor::ivExtractionCompleted(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);
  Q_UNUSED(exitStatus);
  
  m_output->hide();
  QMessageBox::information(this, tr("I-vectors extraction"), tr("I-vectors successfully extracted"));
}

void AudioProcessor::extractMusicRate(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);
  
  QString program;

  if (exitStatus == QProcess::NormalExit) {
    
    // extracting music features
    program = "sh";

    qDebug() << "Extracting music rate...";
    m_musicTrackingProcess->start(program, m_musicTrackArgs);
  }
  else
    QMessageBox::critical(this, tr("Audio file extraction"), tr("An Error occurred while extracting audio file"));
}

void AudioProcessor::retrieveMusicRate(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);

  if (exitStatus == QProcess::NormalExit) {
    mat M;
    M.load("musicTracking/output/M.dat");
    
    int sampleRate = m_musicTrackArgs[2].toInt();
    int mtWindowSize = m_musicTrackArgs[3].toInt();
    int mtHopSize = m_musicTrackArgs[4].toInt();

    qint64 step = static_cast<qreal>(mtHopSize) / sampleRate * 1000;
    qint64 size = static_cast<qreal>(mtWindowSize) / sampleRate * 1000;

    qint64 first(size / 2);
    for (uword i(0); i < M.n_rows; i++) {
      QPair<qint64, qreal> musicRate(first + i * step, M(i, 0));
      emit musicRateRetrieved(musicRate);
    }
    
    QFile::remove("musicTracking/output/M.dat");
    QFile::remove("musicTracking/data/wav/audioFile.wav");

    QMessageBox::information(this, tr("Music tracking"), tr("Musical features successfully extracted"));
  }
  else
    QMessageBox::critical(this, tr("Musical features extraction"), tr("An Error occurred while extracting musical features"));
}

void AudioProcessor::parameterizeOutput() 
{
  QRegularExpression re("(\\d+)_(\\d+)_(\\d+)");
  char buf[1024];
  qint64 lineLength = m_paramProcess->readLine(buf, sizeof(buf));

  if (lineLength != -1) {
    QString outputText = QString::fromLocal8Bit(buf);
    QRegularExpressionMatch match = re.match(outputText);
    
    if (match.hasMatch()) {
      QString outputText = 
	"<b>Parameterizing</b>: Season " +
	match.captured(1) +
	" - Episode " +
	match.captured(2) +
	" - Segment " +
	match.captured(3);

      m_output->setText(outputText);
      m_output->setAlignment(Qt::AlignCenter);
    }
  }
}

void AudioProcessor::normalizeOutput() 
{
  QRegularExpression re("Writing to: (\\d+)_(\\d+)_(\\d+)");
  char buf[1024];
  qint64 lineLength = m_normProcess->readLine(buf, sizeof(buf));
  if (lineLength != -1) {
    QString outputText = QString::fromLocal8Bit(buf);
    QRegularExpressionMatch match = re.match(outputText);

    if (match.hasMatch()) {

      QString outputText = 
	"<b>Normalizing</b> parameters: Season " +
	match.captured(1) +
	" - Episode " +
	match.captured(2) +
	" - Segment " +
	match.captured(3);

      m_output->setText(outputText);
      m_output->setAlignment(Qt::AlignCenter);
    }
  }
}

void AudioProcessor::tvEstimateOutput() 
{
  QRegularExpression re("for \\[(\\d+)_(\\d+)_(\\d+)_\\d+_\\d+\\]");
  char buf[1024];
  qint64 lineLength = m_normProcess->readLine(buf, sizeof(buf));
  if (lineLength != -1) {
    QString outputText = QString::fromLocal8Bit(buf);
    QRegularExpressionMatch match = re.match(outputText);

    if (match.hasMatch()) {

      QString outputText = 
	"<b>Total variability matrix</b> estimation...<br />Processing: Season " +
	match.captured(1) +
	" - Episode " +
	match.captured(2) +
	" - Segment " +
	match.captured(3);

      m_output->setText(outputText);
      m_output->setAlignment(Qt::AlignCenter);
    }
  }
}

void AudioProcessor::genIVectorsOutput()
{
  QRegularExpression re("for \\[(\\d+)_(\\d+)_(\\d+)_\\d+_\\d+\\]");
  char buf[1024];
  qint64 lineLength = m_normProcess->readLine(buf, sizeof(buf));
  if (lineLength != -1) {
    QString outputText = QString::fromLocal8Bit(buf);
    QRegularExpressionMatch match = re.match(outputText);

    if (match.hasMatch()) {

      QString outputText = 
	"<b>Generating i-vectors</b>...<br />Processing: Season " +
	match.captured(1) +
	" - Episode " +
	match.captured(2) +
	" - Segment " +
	match.captured(3);

      m_output->setText(outputText);
      m_output->setAlignment(Qt::AlignCenter);
    }
  }
}

void AudioProcessor::convertOutput()
{
  char buf[1024];
  qint64 lineLength = m_convertProcess->readLine(buf, sizeof(buf));
  if (lineLength != -1) {
    QString outputText = QString::fromLocal8Bit(buf);
    qDebug() << outputText;
  }
}

void AudioProcessor::musicTrackingOutput()
{
  char buf[1024];
  qint64 lineLength = m_musicTrackingProcess->readLine(buf, sizeof(buf));
  if (lineLength != -1) {
    QString outputText = QString::fromLocal8Bit(buf);
    qDebug() << outputText;
  }
}

///////////////////////
// auxiliary methods //
///////////////////////

bool AudioProcessor::extractAudioFiles(QList<SpeechSegment *> speechSegments)
{
  // creating audio files corresponding to speech segments
  QFile lstFile("spkDiarization/data/data.lst");
  QFile totVarFile("spkDiarization/ndx/totalvariability.ndx");
  QFile ivExtFile("spkDiarization/ndx/ivExtractor.ndx");
    
  if (!lstFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;

  if (!totVarFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;

  if (!ivExtFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;

  QTextStream lstOut(&lstFile);
  QTextStream totVarOut(&totVarFile);
  QTextStream ivExtOut(&ivExtFile);

  // retrieve series
  Series *series = retrieveSeries(speechSegments, m_seriesName);

  // retrieve audio data
  QMap<int, QMap<int, QString> > videoFiles;
  QMap<int, QMap<int, QStringList> > lstLines;
  QMap<int, QMap<int, QProcess *> > aviToWaveProcesses;
  int count(0);
  retrieveAudioData(series, videoFiles, aviToWaveProcesses, lstLines, count);

  // load (possibly existing) matrix containing i-vectors
  QString ivFName = "spkDiarization/iv/X_" + m_seriesName + ".dat";
  bool XExists = m_X.load(ivFName.toStdString());
  bool consistent = false;

  // check if X is consistent with current data
  if (XExists) {
    int nbStoredSeg = m_X.n_rows;
    int nbCurrSeg = 0;

    QMap<int, QMap<int, QStringList> >::const_iterator it1 = lstLines.begin();
    while (it1 != lstLines.end()) {

      int seasNbr = it1.key();
      QMap<int, QStringList>::const_iterator it2 = it1.value().begin();
    
      while (it2 != it1.value().end()) {

	int epNbr = it2.key();
	nbCurrSeg += lstLines[seasNbr][epNbr].size();
			   
	it2++;
      }

      it1++;
    }

    consistent = (nbStoredSeg == nbCurrSeg);
    
    if (!consistent) {
      int answer = QMessageBox::question(this, "Inconsistency issue", "Number of stored i-vectors (" + QString::number(nbStoredSeg) + ") is not consistent with current number of speech segments (" + QString::number(nbCurrSeg) + ").<br /><b>Extract i-vectors?</b>", QMessageBox::Yes | QMessageBox::No);

      if (answer == QMessageBox::No)
	consistent = true;
    }
  }

  // if stored i-vectors are consistent with current number of speech segments
  if (XExists && consistent)
    return false;

  // extracting audio files
  QString program;
  QStringList arguments;

  // generating .wav files
  QMap<int, QMap<int, QString> >::const_iterator it1 = videoFiles.begin();

  while (it1 != videoFiles.end()) {

    int seasNbr = it1.key();
    QMap<int, QString>::const_iterator it2 = it1.value().begin();
    
    while (it2 != it1.value().end()) {

      int epNbr = it2.key();
      QString videoFile = it2.value();

      QString waveFile = 
	"spkDiarization/data/sph/" +
	QString::number(seasNbr) + "_" +
	QString::number(epNbr) + ".wav";

      // qDebug() << seasNbr << epNbr << videoFile << waveFile;

      program = "ffmpeg";
      arguments << "-i" << videoFile << "-map" << "0:1" << "-vn" << "-acodec" << "pcm_s16le" <<"-ar" << "16000" << "-ac" << "2" << waveFile;
      aviToWaveProcesses[seasNbr][epNbr]->start(program, arguments);
      aviToWaveProcesses[seasNbr][epNbr]->waitForFinished();
      arguments.clear();

      it2++;
    }
    it1++;
  }

  // generating .sph files
  it1 = videoFiles.begin();
    
  while (it1 != videoFiles.end()) {

    int seasNbr = it1.key();
    QMap<int, QString>::const_iterator it2 = it1.value().begin();
    
    while (it2 != it1.value().end()) {

      int epNbr = it2.key();

      QString waveFile = 
	"spkDiarization/data/sph/" +
	QString::number(seasNbr) + "_" +
	QString::number(epNbr) + ".wav";

      QString sphFile =
      "spkDiarization/data/sph/" +
	QString::number(seasNbr) + "_" +
	QString::number(epNbr) + ".sph";

      // converting .wav audio file to .sph audio file
      // qDebug() << seasNbr << epNbr << waveFile << sphFile;

      QProcess wavToSphProcess;
      program = "sox";
      arguments << waveFile << sphFile;
      wavToSphProcess.start(program, arguments);
      wavToSphProcess.waitForFinished();
      arguments.clear();
      QFile::remove(waveFile);

      // extraction audio files corresponding to speech segments
      QStringList segmentsList = lstLines[seasNbr][epNbr];

      // RE used to retrieve segment time boundaries
      QRegularExpression re("\\d+_\\d+_\\d+_(\\d+)_(\\d+)");

      for (int i(0); i < segmentsList.size(); i++) {

	QString lstLine = segmentsList[i];

	// writing out current segment in data files
	lstOut << lstLine << endl;
	totVarOut << lstLine << endl;
	ivExtOut << lstLine << " " << lstLine << endl;

	QRegularExpressionMatch match = re.match(lstLine);
	
	qreal start(-1.0);
	qreal end(-1.0);
	
	if (match.hasMatch()) {
	  start = ((match.captured(1)).toDouble()) / 1000.0;
	  end = ((match.captured(2)).toDouble()) / 1000.0;
	}

	// extracting .sph speech segments
	QString sphSubFile = "spkDiarization/data/sph/" + lstLine + ".sph";
	// qDebug() << sphSubFile;

	QProcess splitProcess;
	program = "sox";
	arguments << sphFile << sphSubFile << "trim" << QString::number(start) << "=" + QString::number(end);
	splitProcess.start(program, arguments);
	splitProcess.waitForFinished();
	arguments.clear();
      }

      QFile::remove(sphFile);

      it2++;
    }
    it1++;
  }
  
  return true;
}

Series * AudioProcessor::retrieveSeries(QList<SpeechSegment *> speechSegments, QString &name)
{
  Series *series(0);

  if (!speechSegments.isEmpty()) {
    series = dynamic_cast<Series *>(speechSegments[0]->parent()->parent()->parent());
    name = series->getName();
    name.replace(" ", "_");
    name = name.toLower();
  }

  return series;
}

void AudioProcessor::retrieveAudioData(Segment *segment, QMap<int, QMap<int, QString> > &videoFiles, QMap<int, QMap<int, QProcess *> > &aviToWaveProcesses, QMap<int, QMap<int, QStringList> > &lstLines, int &count)
{
  Episode *episode;

  if ((episode = dynamic_cast<Episode *>(segment))) {

    // retrieve current episode features
    int seasNbr = episode->parent()->getNumber();
    int epNbr = episode->getNumber();
    QString epFName = episode->getFName();

    // retrieve and filter speech segments
    QList<SpeechSegment *> speechSegments = episode->getSpeechSegments();
    speechSegments = denoiseSpeechSegments(speechSegments);
    speechSegments = filterSpeechSegments(speechSegments);

    // looping over speech segments
    QString lstLine;

    for (int i(0); i < speechSegments.size(); i++) {

      lstLine = 
	QString::number(seasNbr) + "_" +
	QString::number(epNbr) + "_" +
	QString::number(i + 1) + "_" +
	QString::number(speechSegments[i]->getPosition()) + "_" +
	QString::number(speechSegments[i]->getEnd());

      lstLines[seasNbr][epNbr].push_back(lstLine);

      QString speaker = speechSegments[i]->getLabel(Segment::Manual);
      m_spkIdx[speaker].push_back(count + i);
    }

    // setting video file and episode boundaries if speech segments were found
    if (speechSegments.size() > 0) {

      videoFiles[seasNbr][epNbr] = epFName;
      aviToWaveProcesses[seasNbr][epNbr] = new QProcess;

      m_epBound[seasNbr][epNbr] = QPair<int, int>(count,
						  count + speechSegments.size() - 1);
      count += speechSegments.size();
    }
  }

  else
    for (int i(0); i < segment->childCount(); i++)
      retrieveAudioData(segment->child(i), videoFiles, aviToWaveProcesses, lstLines, count);
}

QList<SpeechSegment *> AudioProcessor::denoiseSpeechSegments(QList<SpeechSegment *> speechSegments)
{
  QList<SpeechSegment *> denoised;

  for (int i(0); i < speechSegments.size(); i++)
    if (speechSegments[i]->getLabel(Segment::Manual) != "")
      denoised.push_back(speechSegments[i]);
  
  return denoised;
}

QList<SpeechSegment *> AudioProcessor::filterSpeechSegments(QList<SpeechSegment *> speechSegments)
{
  QList<SpeechSegment *> filtered;

  for (int i(0); i < speechSegments.size(); i++)
    if (speechSegments[i]->getLabel(Segment::Manual) != "S")
      filtered.push_back(speechSegments[i]);
  
  return filtered;
}
