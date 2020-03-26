#include <QGroupBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QDialogButtonBox>

#include "SceneExtractDialog.h"

SceneExtractDialog::SceneExtractDialog(QWidget *parent)
  : QDialog(parent),
    m_vSrc(Segment::Manual)
{
  setWindowTitle(tr("Extract scenes"));

  QGroupBox *vSrcBox = new QGroupBox("Visual source:");
  QRadioButton *vAuto = new QRadioButton("Automatic");
  QRadioButton *vMan = new QRadioButton("Manual");
  vMan->setChecked(true);
  QGridLayout *vSrcLayout = new QGridLayout;
  vSrcLayout->addWidget(vAuto, 0, 0);
  vSrcLayout->addWidget(vMan, 0, 1);
  vSrcBox->setLayout(vSrcLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(vSrcBox, 0, 0);
  gridLayout->addWidget(buttonBox, 1, 0);

  connect(vAuto, SIGNAL(pressed()), this, SLOT(activAutoVSrc()));
  connect(vMan, SIGNAL(pressed()), this, SLOT(activManVSrc()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  setLayout(gridLayout);
}


///////////
// slots //
///////////

void SceneExtractDialog::activAutoVSrc()
{
  m_vSrc = Segment::Automatic;
}

void SceneExtractDialog::activManVSrc()
{
  m_vSrc = Segment::Manual;
}


///////////////
// accessors //
///////////////

Segment::Source SceneExtractDialog::getVisualSource() const
{
  return m_vSrc;
}
