#include <QGroupBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QGridLayout>

#include "FaceDetectDialog.h"

FaceDetectDialog::FaceDetectDialog(const QString &title, QWidget *parent)
  : QDialog(parent),
    m_method(FaceDetectDialog::OpenCV)
{
  setWindowTitle(title);

  QGroupBox *methBox = new QGroupBox("Face detection method:");
  QRadioButton *openCV = new QRadioButton("OpenCV");
  QRadioButton *zhu = new QRadioButton("Zhu Face Detector");
  QRadioButton *extData = new QRadioButton("External data");
  openCV->setChecked(true);
  QGridLayout *methLayout = new QGridLayout;
  methLayout->addWidget(openCV, 0, 0);
  methLayout->addWidget(zhu, 1, 0);
  methLayout->addWidget(extData, 2, 0);
  methBox->setLayout(methLayout);

  m_minHeightLabel = new QLabel(tr("<b>Minimum face height (%):</b>"));
  m_minHeightSB = new QSpinBox;
  m_minHeightSB->setMinimum(1);
  m_minHeightSB->setValue(16);
  m_minHeightSB->setMaximum(100);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(methBox, 0, 0);
  gridLayout->addWidget(m_minHeightLabel, 1, 0);
  gridLayout->addWidget(m_minHeightSB, 1, 1);
  gridLayout->addWidget(buttonBox, 2, 0, 1, 2, Qt::AlignHCenter);

  setLayout(gridLayout);

  // connecting signals to corresponding slots
  connect(openCV, SIGNAL(clicked()), this, SLOT(activOpenCV()));
  connect(zhu, SIGNAL(clicked()), this, SLOT(activZhu()));
  connect(extData, SIGNAL(clicked()), this, SLOT(activExtData()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  
  connect(openCV, SIGNAL(clicked(bool)), m_minHeightLabel, SLOT(setEnabled(bool)));
  connect(openCV, SIGNAL(clicked(bool)), m_minHeightSB, SLOT(setEnabled(bool)));
  connect(extData, SIGNAL(clicked(bool)), m_minHeightLabel, SLOT(setDisabled(bool)));
  connect(extData, SIGNAL(clicked(bool)), m_minHeightSB, SLOT(setDisabled(bool)));
}

void FaceDetectDialog::activOpenCV()
{
  m_method = FaceDetectDialog::OpenCV;
}

void FaceDetectDialog::activZhu()
{
  m_method = FaceDetectDialog::Zhu;
}

void FaceDetectDialog::activExtData()
{
  m_method = FaceDetectDialog::ExtData;
}

///////////////
// accessors //
///////////////

FaceDetectDialog::Method FaceDetectDialog::getMethod() const
{
  return m_method;
}

int FaceDetectDialog::getMinHeight() const
{
  return m_minHeightSB->value();
}
