#ifndef SPKDIARMONITOR_H
#define SPKDIARMONITOR_H

#include <QWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLabel>
#include <QTextEdit>

#include <armadillo>

#include "UtteranceTreeWidget.h"
#include "SpeechSegment.h"

class SpkDiarMonitor: public QWidget
{
  Q_OBJECT
  
 public:
  SpkDiarMonitor(int treeWidth = 480, int treeHeight = 200, bool global = false, QWidget *parent = 0);
  void exportResults();

  public slots:
    void setDiarData(const arma::mat &E, const arma::mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments);
    void setSpks(QList<int> spkIdx);
    void setSpeakers(QList<QString> speakers, QMap<QString, qreal> spkWeight);
    void getCurrentPattern(const QPair<int, int> &lsuSpeechBound);
    void activL2();
    void activMahal();
    void activSigmaInv();
    void activWInv();
    void normalizeVectors(bool checked);
    void activMin();
    void activMax();
    void activMean();
    void activWard();
    void activSilhouette();
    void activBipartition();
    void playSubtitle(QList<int> utter);
    void currentSubtitle(int subIdx);
    void constrainClustering(bool checked);
    void releasePosition(bool released);
    void setLocalDer(const QString &score);
    void setGlobalDer(const QString &score);

 signals:
    void updateUtteranceTree(const arma::mat &S, const arma::mat &W, const arma::umat &V, const arma::mat &SigmaInv);
    void updateUtteranceTreeShot1(const arma::mat &S, const arma::mat &W, const arma::umat &V, const arma::mat &SigmaInv);
    void updateUtteranceTreeShot2(const arma::mat &S, const arma::mat &W, const arma::umat &V, const arma::mat &SigmaInv);
    void clearUtteranceTree();
    void setDistance(UtteranceTree::DistType dist);
    void setCovInv(const arma::mat &SigmaInv);
    void normVectors(bool checked);
    void setAgrCrit(UtteranceTree::AgrCrit agr);
    void setPartitionMethod(UtteranceTree::PartMeth partMeth);
    void setWeight(const arma::mat &W);
    void playSegments(QList<QPair<qint64, qint64> > segments);
    void currSubtitle(int subIdx);
    void setDiff(const arma::mat &Diff);
    void setSpeakerPartition(QList<QList<int>> partition);
    void releasePos(bool released);

private:
    QVector<QString> m_speakers;
    QMap<QString, qreal> m_spkWeight;
    QLabel *m_locDer;
    QLabel *m_globDer;
    QTextEdit *m_spks;
    QRadioButton *m_l2;
    QRadioButton *m_mahal;
    QGroupBox *m_covBox;
    QCheckBox *m_norm;
    UtteranceTreeWidget *m_utterTree;
    QCheckBox *m_weight;
    arma::mat E;
    arma::mat CovInv;
    arma::mat SigmaInv;
    arma::mat WInv;

QList<SpeechSegment *> m_speechSegments;
QPair<int, int> m_lsuSpeechBound;
};

#endif
