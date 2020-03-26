#include <QGroupBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>

#include <QDebug>

#include "SpkInteractDialog.h"

SpkInteractDialog::SpkInteractDialog(const QString &title, QWidget *parent)
  : QDialog(parent),
    m_type(SpkInteractDialog::Sequential)
{
  setWindowTitle(title);

  QGroupBox *typeBox = new QGroupBox("Interaction estimation:");
  QRadioButton *coOccurr = new QRadioButton("Co-coccurrence based");
  QRadioButton *sequential = new QRadioButton("Sequential");
  sequential->setChecked(true);
  QGridLayout *typeLayout = new QGridLayout;
  typeLayout->addWidget(coOccurr, 0, 0);
  typeLayout->addWidget(sequential, 1, 0);
  typeBox->setLayout(typeLayout);

  QGroupBox *unitBox = new QGroupBox("Reference unit:");
  QRadioButton *scene = new QRadioButton("Scene");
  QRadioButton *lsu = new QRadioButton("Logical Story Unit");
  scene->setChecked(true);
  QGridLayout *unitLayout = new QGridLayout;
  unitLayout->addWidget(scene, 0, 0);
  unitLayout->addWidget(lsu, 1, 0);
  unitBox->setLayout(unitLayout);

  QLabel *nbDiscard = new QLabel(tr("<b>Final utterances to discard</b>"));
  m_nbDiscardSB = new QSpinBox;
  m_nbDiscardSB->setMinimum(0);
  m_nbDiscardSB->setValue(0);
  m_nbDiscardSB->setMaximum(3);

  QLabel *interThresh = new QLabel(tr("<b>Interaction threshold (s.)</b>"));
  m_interThreshSB = new QSpinBox;
  m_interThreshSB->setMinimum(1);
  m_interThreshSB->setValue(5);
  m_interThreshSB->setMaximum(10);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
						     QDialogButtonBox::Cancel);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(typeBox, 0, 0);
  layout->addWidget(unitBox, 1, 0);
  layout->addWidget(nbDiscard, 2, 0);
  layout->addWidget(m_nbDiscardSB, 2, 1);
  layout->addWidget(interThresh, 3, 0);
  layout->addWidget(m_interThreshSB, 3, 1);
  layout->addWidget(buttonBox, 4, 0, 1, 2, Qt::AlignHCenter);

  setLayout(layout);

  // connecting signals to corresponding slots
  connect(sequential, SIGNAL(clicked(bool)), interThresh, SLOT(setEnabled(bool)));
  connect(sequential, SIGNAL(clicked(bool)), m_interThreshSB, SLOT(setEnabled(bool)));
  connect(coOccurr, SIGNAL(clicked(bool)), interThresh, SLOT(setDisabled(bool)));
  connect(coOccurr, SIGNAL(clicked(bool)), m_interThreshSB, SLOT(setDisabled(bool)));

  connect(coOccurr, SIGNAL(clicked()), this, SLOT(activCoOccurr()));
  connect(sequential, SIGNAL(clicked()), this, SLOT(activSequential()));
  connect(scene, SIGNAL(clicked()), this, SLOT(activScene()));
  connect(lsu, SIGNAL(clicked()), this, SLOT(activLSU()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

///////////
// slots //
///////////

void SpkInteractDialog::activCoOccurr()
{
  m_type = SpkInteractDialog::CoOccurr;
}

void SpkInteractDialog::activSequential()
{
  m_type = SpkInteractDialog::Sequential;
}

void SpkInteractDialog::activScene()
{
  m_unit = SpkInteractDialog::Scene;
}

void SpkInteractDialog::activLSU()
{
  m_unit = SpkInteractDialog::LSU;
}

///////////////
// accessors //
///////////////

SpkInteractDialog::InteractType SpkInteractDialog::getInteractType() const
{
  return m_type;
}
 
SpkInteractDialog::RefUnit SpkInteractDialog::getRefUnit() const
{
  return m_unit;
}

int SpkInteractDialog::getNbDiscard() const
{
  return m_nbDiscardSB->value();
}

int SpkInteractDialog::getInterThresh() const
{
  return m_interThreshSB->value();
}
