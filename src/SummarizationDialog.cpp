#include <QGroupBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

#include <QDebug>

#include "SummarizationDialog.h"

SummarizationDialog::SummarizationDialog(const QString &title, const QStringList &seasons, const QStringList &allSpeakers, const QList<QStringList> &seasonSpeakers, QWidget *parent)
  : QDialog(parent),
    m_allSpeakers(allSpeakers),
    m_seasonSpeakers(seasonSpeakers),
    m_method(SummarizationDialog::Both)
{
  setWindowTitle(title);

  QGroupBox *typeBox = new QGroupBox("Type:");
  QRadioButton *global = new QRadioButton("Global");
  QRadioButton *character = new QRadioButton("Character-based");
  character->setChecked(true);
  QGridLayout *typeLayout = new QGridLayout;
  typeLayout->addWidget(global, 0, 0);
  typeLayout->addWidget(character, 1, 0);
  typeBox->setLayout(typeLayout);

  QGroupBox *methodBox = new QGroupBox("Method:");
  QRadioButton *content = new QRadioButton("Content-based");
  QRadioButton *style = new QRadioButton("Style-based");
  QRadioButton *both = new QRadioButton("Both");
  QRadioButton *random = new QRadioButton("Random");
  both->setChecked(true);
  QGridLayout *methodLayout = new QGridLayout;
  methodLayout->addWidget(content, 0, 0);
  methodLayout->addWidget(style, 1, 0);
  methodLayout->addWidget(both, 2, 0);
  methodLayout->addWidget(random, 3, 0);
  methodBox->setLayout(methodLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);
  QLabel *seasonLabel = new QLabel(tr("<b>Seasons:</b>"));
  m_seasonCB = new QComboBox;
  m_seasonCB->addItems(seasons);
  m_seasonCB->setCurrentIndex(seasons.size()-1);

  QLabel *charLabel = new QLabel(tr("<b>Character:</b>"));
  m_charCB = new QComboBox;
  m_charCB->addItems(allSpeakers);

  QLabel *durLabel = new QLabel(tr("<b>Duration / seq. (s.):</b>"));
  m_durSB = new QSpinBox;
  m_durSB->setMinimum(1);
  m_durSB->setMaximum(180);
  m_durSB->setValue(25);
  
  QLabel *granuLabel = new QLabel(tr("<b>Granularity:</b>"));
  m_granuSB = new QSpinBox;
  m_granuSB->setMinimum(0);
  m_granuSB->setMaximum(200);
  m_granuSB->setValue(100);

  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(typeBox, 0, 0);
  gridLayout->addWidget(methodBox, 1, 0);
  gridLayout->addWidget(seasonLabel, 2, 0);
  gridLayout->addWidget(m_seasonCB, 2, 1, 1, 3);
  gridLayout->addWidget(charLabel, 3, 0);
  gridLayout->addWidget(m_charCB, 3, 1, 1, 3);
  gridLayout->addWidget(durLabel, 4, 0);
  gridLayout->addWidget(m_durSB, 4, 3);
  gridLayout->addWidget(granuLabel, 5, 0);
  gridLayout->addWidget(m_granuSB, 5, 3);
  gridLayout->addWidget(buttonBox, 6, 0, 1, 4, Qt::AlignHCenter);

  setLayout(gridLayout);

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  connect(global, SIGNAL(clicked(bool)), charLabel, SLOT(setDisabled(bool)));
  connect(global, SIGNAL(clicked(bool)), m_charCB, SLOT(setDisabled(bool)));
  connect(character, SIGNAL(clicked(bool)), charLabel, SLOT(setEnabled(bool)));
  connect(character, SIGNAL(clicked(bool)), m_charCB, SLOT(setEnabled(bool)));
  
  connect(m_seasonCB, SIGNAL(currentIndexChanged(int)), this, SLOT(seasonChanged(int)));

  connect(content, SIGNAL(clicked()), this, SLOT(activContent()));
  connect(style, SIGNAL(clicked()), this, SLOT(activStyle()));
  connect(both, SIGNAL(clicked()), this, SLOT(activBoth()));
  connect(random, SIGNAL(clicked()), this, SLOT(activRandom()));
}

///////////
// slots //
///////////

void SummarizationDialog::seasonChanged(int index)
{
  /*
  m_charCB->clear();

  if (index == 0)
    m_charCB->addItems(m_allSpeakers);
  else
    m_charCB->addItems(m_seasonSpeakers[index-2]);
  */
}

void SummarizationDialog::activContent()
{
  m_method = SummarizationDialog::Content;
}

void SummarizationDialog::activStyle()
{
  m_method = SummarizationDialog::Style;
}

void SummarizationDialog::activBoth()
{
  m_method = SummarizationDialog::Both;
}

void SummarizationDialog::activRandom()
{
  m_method = SummarizationDialog::Random;
}

///////////////
// accessors //
///////////////

int SummarizationDialog::getSeasonNb() const
{
  /*
  if (m_seasonCB->currentIndex() == 0)
  return -1;
  */

  return m_seasonCB->currentIndex();
}

QString SummarizationDialog::getSpeaker() const
{
  if (m_charCB->isEnabled())
    return m_charCB->currentText();

  return QString();
}

int SummarizationDialog::getDur() const
{
  return m_durSB->value();
}

qreal SummarizationDialog::getGranu() const
{
  return m_granuSB->value() / 100.0;
}

SummarizationDialog::Method SummarizationDialog::getMethod() const
{
  return m_method;
}
