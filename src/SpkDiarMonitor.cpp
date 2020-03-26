#include <QGridLayout>
#include <QPushButton>
#include <QDebug>
#include <QRegularExpression>
#include <QTime>

#include "SpkDiarMonitor.h"

using namespace arma;

SpkDiarMonitor::SpkDiarMonitor(int treeWidth, int treeHeight, bool global, QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *derBox = new QGroupBox("Diarization Error Rate:");
  m_locDer = new QLabel("");
  m_globDer = new QLabel("");
  QGridLayout *derLayout = new QGridLayout;
  derLayout->addWidget(m_locDer, 0, 0);
  if (global)
    derLayout->addWidget(m_globDer, 1, 0);
  derBox->setLayout(derLayout);

  QGroupBox *speakerBox = new QGroupBox("Current speakers:");
  m_spks = new QTextEdit("None");
  m_spks->setReadOnly(true);
  QGridLayout *speakerLayout = new QGridLayout;
  speakerLayout->addWidget(m_spks, 0, 0);
  speakerBox->setLayout(speakerLayout);

  QGroupBox *distBox = new QGroupBox("Distance between utterance vectors:");
  m_l2 = new QRadioButton("L2");
  m_mahal = new QRadioButton("Mahalanobis");
  m_mahal->setChecked(true);
  QGridLayout *distLayout = new QGridLayout;
  distLayout->addWidget(m_l2, 0, 0);
  distLayout->addWidget(m_mahal, 0, 1);
  distBox->setLayout(distLayout);

  m_norm = new QCheckBox(tr("Normalize vectors"));
  m_norm->setChecked(true);

  m_covBox = new QGroupBox("Covariance matrix:");
  QChar sigmaChar(0xa3, 0x03);
  QRadioButton *sigma = new QRadioButton(sigmaChar);
  QRadioButton *w = new QRadioButton("W");
  w->setChecked(true);

  QGridLayout *covLayout = new QGridLayout;
  covLayout->addWidget(sigma, 0, 0);
  covLayout->addWidget(w, 0, 1);
  m_covBox->setLayout(covLayout);

  QGroupBox *agrBox = new QGroupBox("Aggregation criterion:");
  QRadioButton *min = new QRadioButton("Min.");
  QRadioButton *max = new QRadioButton("Max.");
  QRadioButton *mean = new QRadioButton("Mean");
  QRadioButton *ward = new QRadioButton("Ward");
  ward->setChecked(true);
  QGridLayout *agrLayout = new QGridLayout;
  agrLayout->addWidget(min, 0, 0);
  agrLayout->addWidget(max, 0, 1);
  agrLayout->addWidget(mean, 0, 2);
  agrLayout->addWidget(ward, 0, 3);
  agrBox->setLayout(agrLayout);
  
  QGroupBox *partBox = new QGroupBox("Method used to select the best partition:");
  QRadioButton *sil = new QRadioButton("Silhouette");
  QRadioButton *bip = new QRadioButton("Bipartition");
  sil->setChecked(true);
  QGridLayout *partLayout = new QGridLayout;
  partLayout->addWidget(sil, 0, 0);
  partLayout->addWidget(bip, 0, 1);
  partBox->setLayout(partLayout);

  m_weight = new QCheckBox(tr("Weight Utterances"));
  QCheckBox *constrained = new QCheckBox(tr("Constrain clustering"));

  m_utterTree = new UtteranceTreeWidget(treeWidth, treeHeight);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(m_utterTree, 0, 0, 1, 2);
  gridLayout->addWidget(derBox, 1, 0, 1, 2);
  gridLayout->addWidget(distBox, 3, 0, 1, 2);
  gridLayout->addWidget(m_covBox, 4, 0, 1, 2);
  gridLayout->addWidget(m_norm, 5, 0, 1, 2);
  gridLayout->addWidget(agrBox, 6, 0, 1, 2);
  gridLayout->addWidget(partBox, 7, 0, 1, 2);
  gridLayout->addWidget(m_weight, 8, 0, 1, 2);

  if (global) {
    gridLayout->addWidget(speakerBox, 3, 0, 1, 2);
    gridLayout->addWidget(constrained, 9, 0, 1, 2);
  }

  setLayout(gridLayout);

  connect(this, SIGNAL(setDiff(const arma::mat &)), m_utterTree, SLOT(setDiff(const arma::mat &)));
  connect(this, SIGNAL(updateUtteranceTree(const arma::mat &, const arma::mat &, const arma::umat &, const arma::mat &)), m_utterTree, SLOT(updateUtteranceTree(const arma::mat &,const arma::mat &, const arma::umat &, const arma::mat &)));
  connect(m_l2, SIGNAL(pressed()), this, SLOT(activL2()));
  connect(m_mahal, SIGNAL(pressed()), this, SLOT(activMahal()));
  connect(sigma, SIGNAL(pressed()), this, SLOT(activSigmaInv()));
  connect(w, SIGNAL(pressed()), this, SLOT(activWInv()));
  connect(m_norm, SIGNAL(clicked(bool)), this, SLOT(normalizeVectors(bool)));
  connect(this, SIGNAL(normVectors(bool)), m_utterTree, SLOT(normalizeVectors(bool)));
  connect(min, SIGNAL(pressed()), this, SLOT(activMin()));
  connect(max, SIGNAL(pressed()), this, SLOT(activMax()));
  connect(mean, SIGNAL(pressed()), this, SLOT(activMean()));
  connect(ward, SIGNAL(pressed()), this, SLOT(activWard()));
  connect(sil, SIGNAL(pressed()), this, SLOT(activSilhouette()));
  connect(bip, SIGNAL(pressed()), this, SLOT(activBipartition()));
  connect(constrained, SIGNAL(clicked(bool)), this, SLOT(constrainClustering(bool)));
  connect(this, SIGNAL(setDistance(UtteranceTree::DistType)), m_utterTree, SLOT(setDistance(UtteranceTree::DistType)));
  connect(this, SIGNAL(setCovInv(const arma::mat &)), m_utterTree, SLOT(setCovInv(const arma::mat &)));
  connect(this, SIGNAL(setAgrCrit(UtteranceTree::AgrCrit)), m_utterTree, SLOT(setAgrCrit(UtteranceTree::AgrCrit)));
  connect(this, SIGNAL(setPartitionMethod(UtteranceTree::PartMeth)), m_utterTree, SLOT(setPartitionMethod(UtteranceTree::PartMeth)));
  connect(this, SIGNAL(setWeight(const arma::mat &)), m_utterTree, SLOT(setWeight(const arma::mat &)));
  connect(m_utterTree, SIGNAL(playSubtitle(QList<int>)), this, SLOT(playSubtitle(QList<int>)));
  connect(this, SIGNAL(currSubtitle(int)), m_utterTree, SLOT(setCurrSubtitle(int)));
  connect(m_utterTree, SIGNAL(setSpeakers(QList<int>)), this, SLOT(setSpks(QList<int>)));
  connect(this, SIGNAL(releasePos(bool)), m_utterTree, SLOT(releasePos(bool)));
}

void SpkDiarMonitor::exportResults()
{
  emit setSpeakerPartition(m_utterTree->getPartition());
}

///////////
// slots //
///////////

void SpkDiarMonitor::setSpks(QList<int> spkIdx)
{
  QString text;
  int duration;
  int ms;
  QString timeFormat;

  if (m_speakers.size() > 0) {
    for (int i(0); i < spkIdx.size(); i++) {
      duration = static_cast<int>(m_spkWeight[m_speakers[spkIdx[i]]]);
      ms = static_cast<int>((m_spkWeight[m_speakers[spkIdx[i]]] - floor(m_spkWeight[m_speakers[spkIdx[i]]])) * 100) * 10;

      if (duration >= 60)
	timeFormat = "mm:ss.zzz";
      else
	timeFormat = "ss.zzz";

      QTime totalTime(duration / 3600, (duration / 60) % 60, duration % 60, ms);
      text +=  totalTime.toString(timeFormat) + " - " + m_speakers[spkIdx[i]] + "<br />";
    }
  
    m_spks->setHtml(text);
  }
}

void SpkDiarMonitor::setSpeakers(QList<QString> speakers, QMap<QString, qreal> spkWeight)
{
  m_speakers = speakers.toVector();
  m_spkWeight = spkWeight;
}

void SpkDiarMonitor::setDiarData(const mat &E, const mat &Sigma, const arma::mat &W, QList<SpeechSegment *> speechSegments)
{
  this->E = E;
  SigmaInv = pinv(Sigma);
  WInv = pinv(W);
  CovInv = WInv;
  m_speechSegments = speechSegments;
}

void SpkDiarMonitor::getCurrentPattern(const QPair<int, int> &lsuSpeechBound)
{
  if (lsuSpeechBound != m_lsuSpeechBound) {

    // indices of utterances in current pattern
    int m = lsuSpeechBound.second - lsuSpeechBound.first + 1;
    umat V(1, m);

    for (int i(0); i < m; i++)
      V(0, i) = lsuSpeechBound.first + i;

    // matrix containing utterance i-vectors for current pattern
    mat S = E.rows(V);

    // weighting utterances
    mat W(1, m, fill::ones);

    if (m_weight->isChecked())
      for (int i(0); i < m; i++) {
	qreal duration =
	  (m_speechSegments[lsuSpeechBound.first + i]->getEnd() -
	   m_speechSegments[lsuSpeechBound.first + i]->getPosition()) /
	  1000.0;
	W(0, i) = duration;
      }
    else
      W.ones(1, m);

    W /= accu(W);

    // setting the compatibility matrix
    constrainClustering(false);

    // signal to update
    emit updateUtteranceTree(S, W, V, CovInv);
    m_utterTree->normalizeVectors(m_norm->isChecked());

    m_lsuSpeechBound = lsuSpeechBound;
  }
}

void SpkDiarMonitor::constrainClustering(bool checked)
{
  // pattern labels
  QString pattLabel1;
  QString pattLabel2;

  // matrix of compatibility between speakers
  mat Diff = ones(E.n_rows, E.n_rows);

  // regular expression to retrieve pattern name
  QRegularExpression re("(.+\\d+\\)?)_.+");

  // looping over the speakers labels
  for (int i(0); i < m_speakers.size(); i++) {

    QRegularExpressionMatch match1 = re.match(m_speakers[i]);
    if (match1.hasMatch())
      pattLabel1 = match1.captured(1);

    // setting the diagonal matrix to nan
    Diff(i, i) = datum::inf;

    // setting the compatilities between speakers
    if (checked) {

      for (int j(i+1); j < m_speakers.size(); j++) {

	QRegularExpressionMatch match2 = re.match(m_speakers[j]);
	if (match2.hasMatch())
	  pattLabel2 = match2.captured(1);

	// qDebug() << m_speakers[i] << m_speakers[j] << pattLabel1 << pattLabel2;

	if (pattLabel1 == pattLabel2) {
	  Diff(i, j) = datum::inf;
	  Diff(j, i) = datum::inf;
	}
      }
    }
  }

  emit setDiff(Diff);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activL2()
{
  m_covBox->setEnabled(false);
  emit setDistance(UtteranceTree::L2);
  m_utterTree->normalizeVectors(m_norm->isChecked());
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activMahal()
{
  m_covBox->setEnabled(true);
  emit setDistance(UtteranceTree::Mahal);
  m_utterTree->normalizeVectors(m_norm->isChecked());
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activSigmaInv()
{
  CovInv = SigmaInv;
  emit setCovInv(CovInv);
  m_utterTree->normalizeVectors(m_norm->isChecked());
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activWInv()
{
  CovInv = WInv;
  emit setCovInv(CovInv);
  m_utterTree->normalizeVectors(m_norm->isChecked());
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::normalizeVectors(bool checked)
{
  emit normVectors(checked);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activMin()
{
  emit setAgrCrit(UtteranceTree::Min);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activMax()
{
  emit setAgrCrit(UtteranceTree::Max);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activMean()
{
  emit setAgrCrit(UtteranceTree::Mean);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activWard()
{
  emit setAgrCrit(UtteranceTree::Ward);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activSilhouette()
{
  emit setPartitionMethod(UtteranceTree::Silhouette);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::activBipartition()
{
  emit setPartitionMethod(UtteranceTree::Bipartition);
  emit setSpeakerPartition(m_utterTree->getPartition());
}

void SpkDiarMonitor::playSubtitle(QList<int> utter)
{
  QList<QPair<qint64, qint64> > segments;

  for (int i(0); i < utter.size(); i++)
    segments.push_back(QPair<qint64, qint64>(m_speechSegments[utter[i]]->getPosition(), m_speechSegments[utter[i]]->getEnd()));

  emit playSegments(segments);
}

void SpkDiarMonitor::currentSubtitle(int subIdx)
{
  emit currSubtitle(subIdx);
}

void SpkDiarMonitor::releasePosition(bool released)
{
  emit releasePos(released);
}

void SpkDiarMonitor::setLocalDer(const QString &score)
{
  m_locDer->setText("Local:\t" + score + "%");
}

void SpkDiarMonitor::setGlobalDer(const QString &score)
{
  m_globDer->setText("Global:\t" + score + "%");
}
