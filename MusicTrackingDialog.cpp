#include <QDialogButtonBox>
#include <QLabel>
#include <QGridLayout>

#include "MusicTrackingDialog.h"

MusicTrackingDialog::MusicTrackingDialog(const QString &title, QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(title);

  QLabel *sampleRateLabel = new QLabel(tr("<b>Sample rate:</b>"));
  m_sampleRateSB = new QSpinBox;
  m_sampleRateSB->setMinimum(8000);
  m_sampleRateSB->setMaximum(44100);
  m_sampleRateSB->setValue(16000);
  
  QLabel *mtWindowSizeLabel = new QLabel(tr("<b>Mid-term window size:</b>"));
  m_mtWindowSizeSB = new QSpinBox;
  m_mtWindowSizeSB->setMinimum(m_sampleRateSB->value() * 1);
  m_mtWindowSizeSB->setMaximum(m_sampleRateSB->value() * 10);
  m_mtWindowSizeSB->setValue(m_sampleRateSB->value() * 7);

  QLabel *mtHopSizeLabel = new QLabel(tr("<b>Hop size between measurements:</b>"));
  m_mtHopSizeSB = new QSpinBox;
  m_mtHopSizeSB->setMinimum(m_mtWindowSizeSB->value() / 10);
  m_mtHopSizeSB->setMaximum(m_mtWindowSizeSB->value() / 1);
  m_mtHopSizeSB->setValue(m_mtWindowSizeSB->value() / 7);

  QLabel *chromaStaticFrameSizeLabel = new QLabel(tr("<b>Frame size for chroma (static dispersion):</b>"));
  m_chromaStaticFrameSizeSB = new QSpinBox;
  m_chromaStaticFrameSizeSB->setMinimum(m_sampleRateSB->value() / 100);
  m_chromaStaticFrameSizeSB->setMaximum(m_sampleRateSB->value() / 5);
  m_chromaStaticFrameSizeSB->setValue(m_sampleRateSB->value() / 10);

  QLabel *chromaDynamicFrameSizeLabel = new QLabel(tr("<b>Frame size for chroma (dynamic dispersion):</b>"));
  m_chromaDynamicFrameSizeSB = new QSpinBox;
  m_chromaDynamicFrameSizeSB->setMinimum(m_sampleRateSB->value() / 100);
  m_chromaDynamicFrameSizeSB->setMaximum(m_sampleRateSB->value() / 5);
  m_chromaDynamicFrameSizeSB->setValue(m_sampleRateSB->value() / 20);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
						     QDialogButtonBox::Cancel);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(sampleRateLabel, 0, 0);
  gridLayout->addWidget(m_sampleRateSB, 0, 1);
  gridLayout->addWidget(mtWindowSizeLabel, 1, 0);
  gridLayout->addWidget(m_mtWindowSizeSB, 1, 1);
  gridLayout->addWidget(mtHopSizeLabel, 2, 0);
  gridLayout->addWidget(m_mtHopSizeSB, 2, 1);
  gridLayout->addWidget(chromaStaticFrameSizeLabel, 3, 0);
  gridLayout->addWidget(m_chromaStaticFrameSizeSB, 3, 1);
  gridLayout->addWidget(chromaDynamicFrameSizeLabel, 4, 0);
  gridLayout->addWidget(m_chromaDynamicFrameSizeSB, 4, 1);
  gridLayout->addWidget(buttonBox, 5, 0, 1, 2, Qt::AlignHCenter);

  setLayout(gridLayout);

  // connecting signals to corresponding slots
  connect(m_sampleRateSB, SIGNAL(valueChanged(int)), this, SLOT(adjustExtrema(int)));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

///////////
// slots //
///////////

void MusicTrackingDialog::adjustExtrema(int value)
{
  m_mtWindowSizeSB->setMinimum(value * 1);
  m_mtWindowSizeSB->setMaximum(value * 10);
  m_mtWindowSizeSB->setValue(value * 7);

  m_mtHopSizeSB->setMinimum(m_mtWindowSizeSB->value() / 10);
  m_mtHopSizeSB->setMaximum(m_mtWindowSizeSB->value() / 1);
  m_mtHopSizeSB->setValue(m_mtWindowSizeSB->value() / 7);

  m_chromaStaticFrameSizeSB->setMinimum(m_sampleRateSB->value() / 100);
  m_chromaStaticFrameSizeSB->setMaximum(m_sampleRateSB->value() / 5);
  m_chromaStaticFrameSizeSB->setValue(m_sampleRateSB->value() / 10);

  m_chromaDynamicFrameSizeSB->setMinimum(m_sampleRateSB->value() / 100);
  m_chromaDynamicFrameSizeSB->setMaximum(m_sampleRateSB->value() / 5);
  m_chromaDynamicFrameSizeSB->setValue(m_sampleRateSB->value() / 50);
}

///////////////
// accessors //
///////////////

int MusicTrackingDialog::getSampleRate() const
{
return m_sampleRateSB->value();
}

int MusicTrackingDialog::getMtWindowSize() const
{
  return m_mtWindowSizeSB->value();
}

int MusicTrackingDialog::getMtHopSize() const
{
  return m_mtHopSizeSB->value();
}

int MusicTrackingDialog::getChromaStaticFrameSize() const
{
  return m_chromaStaticFrameSizeSB->value();
}

int MusicTrackingDialog::getChromaDynamicFrameSize() const
{
return m_chromaDynamicFrameSizeSB->value();
}
