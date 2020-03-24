#include <QGroupBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>

#include "SpkDiarizationDialog.h"

SpkDiarizationDialog::SpkDiarizationDialog(const QString &title, bool local, bool view, QWidget *parent)
  : QDialog(parent),
    m_dist(UtteranceTree::Mahal),
    m_agrCrit(UtteranceTree::Ward),
    m_partMeth(UtteranceTree::Silhouette),
    m_method(SpkDiarizationDialog::HC),
    m_sigma(false),
    m_local(local)
{
  setWindowTitle(title);

  QGroupBox *methBox = new QGroupBox("Local clustering method:");
  QRadioButton *hier = new QRadioButton("Bottom-Up hierarchical clustering");
  QRadioButton *refSpk = new QRadioButton("Reference speakers");
  hier->setChecked(true);
  QGridLayout *methLayout = new QGridLayout;
  methLayout->addWidget(hier, 0, 0);
  methLayout->addWidget(refSpk, 1, 0);
  methBox->setLayout(methLayout);

  m_ubm = new QCheckBox(tr("New GMM/UBM"));
  m_ubm->hide();

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

  QGroupBox *covBox = new QGroupBox("Covariance matrix:");
  QChar sigmaChar(0xa3, 0x03);
  QRadioButton *sigma = new QRadioButton(sigmaChar);
  QRadioButton *w = new QRadioButton("W");
  w->setChecked(true);
  QGridLayout *covLayout = new QGridLayout;
  covLayout->addWidget(sigma, 0, 0);
  covLayout->addWidget(w, 0, 1);
  covBox->setLayout(covLayout);
  covBox->setEnabled(true);

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
  agrBox->setEnabled(true);

  QGroupBox *partBox = new QGroupBox("Method used to select the best partition:");
  QRadioButton *sil = new QRadioButton("Silhouette");
  QRadioButton *bip = new QRadioButton("Bipartition");
  sil->setChecked(true);
  QGridLayout *partLayout = new QGridLayout;
  partLayout->addWidget(sil, 0, 0);
  partLayout->addWidget(bip, 0, 1);
  partBox->setLayout(partLayout);
  partBox->setEnabled(true);

  m_weight = new QCheckBox(tr("Weight Utterances"));

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(methBox, 0, 0);
  gridLayout->addWidget(m_ubm, 1, 0);
  gridLayout->addWidget(distBox, 2, 0);
  gridLayout->addWidget(covBox, 3, 0);
  gridLayout->addWidget(m_norm, 4, 0);
  gridLayout->addWidget(agrBox, 5, 0);
  gridLayout->addWidget(partBox, 6, 0);
  gridLayout->addWidget(m_weight, 7, 0);
  gridLayout->addWidget(buttonBox, 8, 0, 1, 2, Qt::AlignHCenter);

  setLayout(gridLayout);

  // hide options in case of view request
  if (view) {
    methBox->hide();
    distBox->hide();
    covBox->hide();
    m_norm->hide();
    agrBox->hide();
    partBox->hide();
    m_weight->hide();
  }

  // connecting signals to corresponding slots
  connect(m_l2, SIGNAL(clicked()), this, SLOT(activL2()));
  connect(m_l2, SIGNAL(clicked(bool)), covBox, SLOT(setDisabled(bool)));
  connect(m_mahal, SIGNAL(clicked()), this, SLOT(activMahal()));
  connect(m_mahal, SIGNAL(clicked(bool)), covBox, SLOT(setEnabled(bool)));
  connect(sigma, SIGNAL(clicked()), this, SLOT(activSigma()));
  connect(w, SIGNAL(clicked()), this, SLOT(activW()));
  connect(min, SIGNAL(clicked()), this, SLOT(activMin()));
  connect(max, SIGNAL(clicked()), this, SLOT(activMax()));
  connect(mean, SIGNAL(clicked()), this, SLOT(activMean()));
  connect(ward, SIGNAL(clicked()), this, SLOT(activWard()));
  connect(sil, SIGNAL(clicked()), this, SLOT(activSilhouette()));
  connect(bip, SIGNAL(clicked()), this, SLOT(activBipartition()));
  connect(hier, SIGNAL(clicked()), this, SLOT(activHier()));
  connect(refSpk, SIGNAL(clicked()), this, SLOT(activRefSpk()));

  connect(refSpk, SIGNAL(clicked(bool)), m_ubm, SLOT(setDisabled(bool)));
  connect(refSpk, SIGNAL(clicked(bool)), distBox, SLOT(setDisabled(bool)));
  connect(refSpk, SIGNAL(clicked(bool)), covBox, SLOT(setDisabled(bool)));
  connect(refSpk, SIGNAL(clicked(bool)), m_norm, SLOT(setDisabled(bool)));
  connect(refSpk, SIGNAL(clicked(bool)), agrBox, SLOT(setDisabled(bool)));
  connect(refSpk, SIGNAL(clicked(bool)), partBox, SLOT(setDisabled(bool)));
  connect(refSpk, SIGNAL(clicked(bool)), m_weight, SLOT(setDisabled(bool)));
  
  connect(hier, SIGNAL(clicked(bool)), m_ubm, SLOT(setEnabled(bool)));
  connect(hier, SIGNAL(clicked(bool)), distBox, SLOT(setEnabled(bool)));
  connect(hier, SIGNAL(clicked(bool)), covBox, SLOT(setEnabled(bool)));
  connect(hier, SIGNAL(clicked(bool)), m_norm, SLOT(setEnabled(bool)));
  connect(hier, SIGNAL(clicked(bool)), agrBox, SLOT(setEnabled(bool)));
  connect(hier, SIGNAL(clicked(bool)), partBox, SLOT(setEnabled(bool)));
  connect(hier, SIGNAL(clicked(bool)), m_weight, SLOT(setEnabled(bool)));

  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

///////////////
// accessors //
///////////////

QString SpkDiarizationDialog::getSpeakersFName() const
{
  return m_speakersFName;
}

UtteranceTree::DistType SpkDiarizationDialog::getDist() const
{
  return m_dist;
}

UtteranceTree::AgrCrit SpkDiarizationDialog::getAgrCrit() const
{
  return m_agrCrit;
}

UtteranceTree::PartMeth SpkDiarizationDialog::getPartMeth() const
{
  return m_partMeth;
}

SpkDiarizationDialog::Method SpkDiarizationDialog::getMethod() const
{
  return m_method;
}

bool SpkDiarizationDialog::getNorm() const
{
  return m_norm->isChecked();
}

bool SpkDiarizationDialog::getWeight() const
{
  return m_weight->isChecked();
}

bool SpkDiarizationDialog::getSigma() const
{
  return m_sigma;
}

bool SpkDiarizationDialog::getUbm() const
{
  return m_ubm->isChecked();
}

///////////
// slots //
///////////

void SpkDiarizationDialog::activL2()
{
  m_dist = UtteranceTree::L2;
}

void SpkDiarizationDialog::activMahal()
{
  m_dist = UtteranceTree::Mahal;
}

void SpkDiarizationDialog::activSigma()
{
  m_sigma = true;
}

void SpkDiarizationDialog::activW()
{
  m_sigma = false;
}

void SpkDiarizationDialog::activMin()
{
  m_agrCrit = UtteranceTree::Min;
}

void SpkDiarizationDialog::activMax()
{
  m_agrCrit = UtteranceTree::Max;
}

void SpkDiarizationDialog::activMean()
{
  m_agrCrit = UtteranceTree::Mean;
}

void SpkDiarizationDialog::activWard()
{
  m_agrCrit = UtteranceTree::Ward;
  update();
}

void SpkDiarizationDialog::activSilhouette()
{
  m_partMeth = UtteranceTree::Silhouette;
}

void SpkDiarizationDialog::activBipartition()
{
  m_partMeth = UtteranceTree::Bipartition;
}

void SpkDiarizationDialog::activHier()
{
  m_method = SpkDiarizationDialog::HC;
}

void SpkDiarizationDialog::activRefSpk()
{
  m_method = SpkDiarizationDialog::Ref;
}
