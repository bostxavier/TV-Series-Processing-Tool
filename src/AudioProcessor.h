#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QFile>
#include <QMap>
#include <QProcess>
#include <QLabel>

#include <armadillo>

#include <QDebug>

#include "Series.h"
#include "SpeechSegment.h"

class AudioProcessor: public QWidget
{
  Q_OBJECT

 public:
  AudioProcessor(QWidget *parent = 0);
  void extractIVectors(QList<SpeechSegment *> speechSegments);
  arma::mat getEpisodeIVectors(QList<SpeechSegment *> speechSegments);
  arma::mat genWMat();
  arma::mat genSigmaMat();
  void trackMusic(const QString &epFName, int frameRate, int mtWindowSize, int mtHopSize, int chromaStaticFrameSize, int chromaDynamicFrameSize);

  QList<SpeechSegment *> denoiseSpeechSegments(QList<SpeechSegment *> speechSegments);
  QList<SpeechSegment *> filterSpeechSegments(QList<SpeechSegment *> speechSegments);

  public slots:
    void parameterizeSegments();
    void normalizeParameters(int exitCode, QProcess::ExitStatus exitStatus);
    void estimateTVMatrix(int exitCode, QProcess::ExitStatus exitStatus);
    void genIVectors(int exitCode, QProcess::ExitStatus exitStatus);
    void genXMatrix(int exitCode, QProcess::ExitStatus exitStatus);
    void cleanDir(int exitCode, QProcess::ExitStatus exitStatus);
    void ivExtractionCompleted(int exitCode, QProcess::ExitStatus exitStatus);
    void extractMusicRate(int exitCode, QProcess::ExitStatus exitStatus);  
    void retrieveMusicRate(int exitCode, QProcess::ExitStatus exitStatus);

    void parameterizeOutput();
    void normalizeOutput();
    void tvEstimateOutput();
    void genIVectorsOutput();
    void convertOutput();
    void musicTrackingOutput();

    signals:
    void musicRateRetrieved(QPair<qint64, qreal> musicRate);
  
    private:
    bool extractAudioFiles(QList<SpeechSegment *> speechSegments);
    void retrieveAudioData(Segment *segment, QMap<int, QMap<int, QString> > &videoFiles, QMap<int, QMap<int, QProcess *> > &aviToWaveProcesses, QMap<int, QMap<int, QStringList> > &lstLines, int &count);
    Series * retrieveSeries(QList<SpeechSegment *> speechSegments, QString &name);
    void extractAudioFile(const QString &epFName, int frameRate);

    QString m_seriesName;
    QProcess *m_paramProcess;
    QProcess *m_normProcess;
    QProcess *m_tvProcess;
    QProcess *m_ivProcess;
    QProcess *m_genXProcess;
    QProcess *m_cleanDirProcess;
    QProcess *m_convertProcess;
    QProcess *m_musicTrackingProcess;

    QLabel *m_output;

    arma::mat m_X;
    QMap<int, QMap<int, QPair<int, int> > > m_epBound;
    QMap<QString, QList<int> > m_spkIdx;

    QStringList m_musicTrackArgs;
};

#endif
