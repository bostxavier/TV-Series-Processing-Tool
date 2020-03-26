#include <QGroupBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QGridLayout>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "CompareImageDialog.h"

using namespace cv;

CompareImageDialog::CompareImageDialog(const QString &title, int V, int H, int S, int defValue1, int defValue2, const QString &thresh1Text, const QString &thresh2Text, QWidget *parent)
  : QDialog(parent),
    m_histoType(2),
    m_metrics(CV_COMP_CORREL)
{
  setWindowTitle(title);

  QGroupBox *histoSrcBox = new QGroupBox("Histogram source:");
  QRadioButton *lRB = new QRadioButton("Luminance");
  QRadioButton *hsRB = new QRadioButton("Hue/Saturation");
  QRadioButton *hsvRB = new QRadioButton("HSV");
  hsvRB->setChecked(true);
  QGridLayout *histoSrcLayout = new QGridLayout;
  histoSrcLayout->addWidget(lRB, 0, 0);
  histoSrcLayout->addWidget(hsRB, 0, 1);
  histoSrcLayout->addWidget(hsvRB, 0, 2);
  histoSrcBox->setLayout(histoSrcLayout);

  QLabel *blockNBinsLabel = new QLabel(tr("<b>Number of histogram bins for each channel:</b>"));
  m_nVBinsLabel = new QLabel(tr("Value"));
  m_nHBinsLabel = new QLabel(tr("Hue"));
  m_nSBinsLabel = new QLabel(tr("Saturation"));
  m_nVBinsSB = new QSpinBox;
  m_nHBinsSB = new QSpinBox;
  m_nSBinsSB = new QSpinBox;
  m_nVBinsSB->setMinimum(2);
  m_nVBinsSB->setValue(V);
  m_nVBinsSB->setMaximum(256);
  m_nHBinsSB->setMinimum(2);
  m_nHBinsSB->setValue(H);
  m_nHBinsSB->setMaximum(180);
  m_nSBinsSB->setMinimum(2);
  m_nSBinsSB->setValue(S);
  m_nSBinsSB->setMaximum(255);
  
  QLabel *blockNumberLabel = new QLabel(tr("<b>Number of subimages along each dimension:</b>"));
  QLabel *nVBlockLabel = new QLabel(tr("Vertical"));
  QLabel *nHBlockLabel = new QLabel(tr("Horizontal"));
  m_nVBlockSB = new QSpinBox;
  m_nHBlockSB = new QSpinBox;
  m_nVBlockSB->setMinimum(1);
  m_nVBlockSB->setValue(5);
  m_nVBlockSB->setMaximum(20);
  m_nHBlockSB->setMinimum(1);
  m_nHBlockSB->setValue(6);
  m_nHBlockSB->setMaximum(20);

  QGroupBox *metricsBox = new QGroupBox("Metrics:");
  QRadioButton *l1 = new QRadioButton("L1");
  QRadioButton *l2 = new QRadioButton("L2");
  QRadioButton *correl = new QRadioButton("Correlation");
  QRadioButton *chisqr = new QRadioButton("Chi-Square");
  QRadioButton *intersect = new QRadioButton("Intersection");
  QRadioButton *hellinger = new QRadioButton("Hellinger dist.");
  correl->setChecked(true);
  QGridLayout *metricsLayout = new QGridLayout;
  metricsLayout->addWidget(l1, 0, 0);
  metricsLayout->addWidget(l2, 0, 1);
  metricsLayout->addWidget(correl, 0, 2);
  metricsLayout->addWidget(chisqr, 1, 0);
  metricsLayout->addWidget(intersect, 1, 1);
  metricsLayout->addWidget(hellinger, 1, 2);
  metricsBox->setLayout(metricsLayout);

  QLabel *thresholdLabel1 = new QLabel(thresh1Text);
  m_thresholdSB1 = new QSpinBox;
  m_thresholdSB1->setMinimum(1);
  m_thresholdSB1->setValue(defValue1);
  m_thresholdSB1->setMaximum(100);

  QLabel *thresholdLabel2 = new QLabel(thresh2Text);
  m_thresholdSB2 = new QSpinBox;
  m_thresholdSB2->setMinimum(0);
  m_thresholdSB2->setValue(defValue2);
  m_thresholdSB2->setMaximum(100);
  
  if (thresh2Text.isEmpty()) {
    m_thresholdSB2->hide();
    thresholdLabel2->hide();
  }

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(histoSrcBox, 0, 0, 1, 3);
  gridLayout->addWidget(blockNBinsLabel, 1, 0, 1, 3);
  gridLayout->addWidget(m_nVBinsLabel, 2, 0);
  gridLayout->addWidget(m_nVBinsSB, 2, 1);
  gridLayout->addWidget(m_nHBinsLabel, 3, 0);
  gridLayout->addWidget(m_nHBinsSB, 3, 1);
  gridLayout->addWidget(m_nSBinsLabel, 4, 0);
  gridLayout->addWidget(m_nSBinsSB, 4, 1);
  gridLayout->addWidget(blockNumberLabel, 5, 0, 1, 3);
  gridLayout->addWidget(nVBlockLabel, 6, 0);
  gridLayout->addWidget(m_nVBlockSB, 6, 1);
  gridLayout->addWidget(nHBlockLabel, 7, 0);
  gridLayout->addWidget(m_nHBlockSB, 7, 1);
  gridLayout->addWidget(metricsBox, 8, 0, 1, 3);
  gridLayout->addWidget(thresholdLabel1, 9, 0);
  gridLayout->addWidget(m_thresholdSB1, 10, 1);
  gridLayout->addWidget(thresholdLabel2, 11, 0);
  gridLayout->addWidget(m_thresholdSB2, 12, 1);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);
  gridLayout->addWidget(buttonBox, 13, 0, 1, 3, Qt::AlignHCenter);

  setLayout(gridLayout);

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  // connecting histo type buttons to corresponding slots
  connect(lRB, SIGNAL(pressed()), this, SLOT(activLHisto()));
  connect(hsRB, SIGNAL(pressed()), this, SLOT(activHsHisto()));
  connect(hsvRB, SIGNAL(pressed()), this, SLOT(activHsvHisto()));

  // connecting metrics buttons to corresponding slots
  connect(l1, SIGNAL(pressed()), this, SLOT(activL1()));
  connect(l2, SIGNAL(pressed()), this, SLOT(activL2()));
  connect(correl, SIGNAL(pressed()), this, SLOT(activCorrel()));
  connect(chisqr, SIGNAL(pressed()), this, SLOT(activChiSqr()));
  connect(intersect, SIGNAL(pressed()), this, SLOT(activIntersect()));
  connect(hellinger, SIGNAL(pressed()), this, SLOT(activHellinger()));
}

///////////
// slots //
///////////

void CompareImageDialog::activLHisto()
{
  m_nVBinsSB->setEnabled(true);
  m_nHBinsSB->setEnabled(false);
  m_nSBinsSB->setEnabled(false);
  m_nVBinsLabel->setEnabled(true);
  m_nHBinsLabel->setEnabled(false);
  m_nSBinsLabel->setEnabled(false);
  m_histoType = 0;
}

void CompareImageDialog::activHsHisto()
{
  m_nVBinsSB->setEnabled(false);
  m_nHBinsSB->setEnabled(true);
  m_nSBinsSB->setEnabled(true);
  m_nVBinsLabel->setEnabled(false);
  m_nHBinsLabel->setEnabled(true);
  m_nSBinsLabel->setEnabled(true);
  m_histoType = 1;
}

void CompareImageDialog::activHsvHisto()
{
  m_nVBinsSB->setEnabled(true);
  m_nHBinsSB->setEnabled(true);
  m_nSBinsSB->setEnabled(true);
  m_nVBinsLabel->setEnabled(true);
  m_nHBinsLabel->setEnabled(true);
  m_nSBinsLabel->setEnabled(true);
  m_histoType = 2;
}

void CompareImageDialog::activL1()
{
  m_metrics = 4;
}

void CompareImageDialog::activL2()
{
  m_metrics = 5;
}

void CompareImageDialog::activCorrel()
{
  m_metrics = CV_COMP_CORREL;
}
 
void CompareImageDialog::activChiSqr()
{
  m_metrics = CV_COMP_CHISQR;
}
 
void CompareImageDialog::activIntersect()
{
  m_metrics = CV_COMP_INTERSECT;
}
 
void CompareImageDialog::activHellinger()
{
  m_metrics = CV_COMP_HELLINGER;
}

///////////////
// accessors //
///////////////

int CompareImageDialog::getHistoType()
{
  return m_histoType;
}

int CompareImageDialog::getMetrics()
{
  return m_metrics;
}

int CompareImageDialog::getThreshold1()
{
  return m_thresholdSB1->value();
}

int CompareImageDialog::getThreshold2()
{
  return m_thresholdSB2->value();
}

int CompareImageDialog::getNVBlock()
{
  return m_nVBlockSB->value();
}

int CompareImageDialog::getNHBlock()
{
  return m_nHBlockSB->value();
}

int CompareImageDialog::getNVBins()
{
  return m_nVBinsSB->value();
}

int CompareImageDialog::getNHBins()
{
  return m_nHBinsSB->value();
}

int CompareImageDialog::getNSBins()
{
  return m_nSBinsSB->value();
}
