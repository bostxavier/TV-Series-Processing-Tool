#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QGroupBox>
#include <QRadioButton>
#include <QDebug>

#include <opencv2/imgproc/imgproc.hpp>

#include "HistoMonitor.h"

using namespace cv;

HistoMonitor::HistoMonitor(QWidget *parent, int hBins, int sBins, int vBins)
  : QWidget(parent),
    m_hBins(hBins),
    m_sBins(sBins),
    m_vBins(vBins)
{
  const int vScale = 12;
  const int diffScale = 8;

  m_processor = new VideoFrameProcessor;

  QSlider *vVScaleSlider = new QSlider(Qt::Vertical);
  vVScaleSlider->setMinimum(1);
  vVScaleSlider->setMaximum(30);
  vVScaleSlider->setValue(vScale);
  m_vHisto = new VHistoWidget(vScale);
  QSlider *vNBinsSlider = new QSlider(Qt::Horizontal);
  vNBinsSlider->setMinimum(2);
  vNBinsSlider->setMaximum(256);
  vNBinsSlider->setValue(m_vBins);
  
  QSlider *satSlider = new QSlider(Qt::Vertical);
  satSlider->setMinimum(2);
  satSlider->setMaximum(255);
  satSlider->setValue(m_sBins);
  m_hsHisto = new HsHistoWidget;
  QSlider *hueSlider = new QSlider(Qt::Horizontal);
  hueSlider->setMinimum(2);
  hueSlider->setMaximum(180);
  hueSlider->setValue(m_hBins);

  QSlider *diffScaleSlider = new QSlider(Qt::Vertical);
  diffScaleSlider->setMinimum(1);
  diffScaleSlider->setMaximum(16);
  diffScaleSlider->setValue(diffScale);
  DiffHistoGraph *hsvGraph = new DiffHistoGraph(Qt::black);

  QGroupBox *groupBox = new QGroupBox("Metrics: ");
  QRadioButton *l1 = new QRadioButton("L1");
  QRadioButton *l2 = new QRadioButton("L2");
  QRadioButton *correl = new QRadioButton("Correlation");
  QRadioButton *chisqr = new QRadioButton("Chi-Square");
  QRadioButton *intersect = new QRadioButton("Intersection");
  QRadioButton *hellinger = new QRadioButton("Hellinger dist.");
  correl->setChecked(true);
  QGridLayout *boxLayout = new QGridLayout;
  boxLayout->addWidget(l1, 0, 0);
  boxLayout->addWidget(l2, 0, 1);
  boxLayout->addWidget(correl, 0, 2);
  boxLayout->addWidget(chisqr, 1, 0);
  boxLayout->addWidget(intersect, 1, 1);
  boxLayout->addWidget(hellinger, 1, 2);
  groupBox->setLayout(boxLayout);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(vVScaleSlider, 0, 0);
  layout->addWidget(m_vHisto, 0, 1);
  layout->addWidget(vNBinsSlider, 1, 1);
  layout->addWidget(satSlider, 0, 2);
  layout->addWidget(m_hsHisto, 0, 3, 1, 1, Qt::AlignHCenter);
  layout->addWidget(hueSlider, 1, 3);
  layout->addWidget(diffScaleSlider, 2, 0);
  layout->addWidget(hsvGraph, 2, 1, 1, 3, Qt::AlignHCenter);
  layout->addWidget(groupBox, 3, 0, 1, 4, Qt::AlignHCenter);

  setLayout(layout);

  // connecting sliders to corresponding slots
  connect(vVScaleSlider, SIGNAL(valueChanged(int)), m_vHisto, SLOT(setScale(int)));
  connect(vNBinsSlider, SIGNAL(valueChanged(int)), this, SLOT(setVBins(int)));
  connect(hueSlider, SIGNAL(valueChanged(int)), this, SLOT(setHBins(int)));
  connect(satSlider, SIGNAL(valueChanged(int)), this, SLOT(setSBins(int)));
  connect(diffScaleSlider, SIGNAL(valueChanged(int)), hsvGraph, SLOT(setScale(int)));

  // connecting metrics buttons to corresponding slots
  connect(l1, SIGNAL(pressed()), m_processor, SLOT(activL1()));
  connect(l2, SIGNAL(pressed()), m_processor, SLOT(activL2()));
  connect(correl, SIGNAL(pressed()), m_processor, SLOT(activCorrel()));
  connect(chisqr, SIGNAL(pressed()), m_processor, SLOT(activChiSqr()));
  connect(intersect, SIGNAL(pressed()), m_processor, SLOT(activIntersect()));
  connect(hellinger, SIGNAL(pressed()), m_processor, SLOT(activHellinger()));

  // updating distance from previous frame for graphing purpose
  connect(this, SIGNAL(distFromPrev(qreal)), hsvGraph, SLOT(appendDist(qreal)));
}

///////////
// slots //
///////////

void HistoMonitor::processMat(const Mat &bgrMat)
{
  Mat hsvMat;
  Mat vHisto, hsHisto, hsvHisto;
  qreal distance;

  cvtColor(bgrMat, hsvMat, CV_BGR2HSV);
  
  vHisto = m_processor->genVHisto(hsvMat, m_vBins);
  hsHisto = m_processor->genHsHisto(hsvMat, m_hBins, m_sBins);
  hsvHisto = m_processor->genHsvHisto(hsvMat, m_hBins, m_sBins, m_sBins);

  m_vHisto->setHisto(vHisto);
  m_hsHisto->setHisto(hsHisto);

  distance = m_processor->distanceFromPrev(hsvHisto, m_prevHsvHisto);
  emit distFromPrev(distance);
  
  m_prevHsvHisto = hsvHisto;
}

void HistoMonitor::setHBins(int hBins)
{
  m_hBins = hBins;
}

void HistoMonitor::setSBins(int sBins)
{
  m_sBins = sBins;
}

void HistoMonitor::setVBins(int vBins)
{
  m_vBins = vBins;
}
