#include <QWidget>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>

#include "NewProjectDialog.h"

/////////////////
// constructor //
/////////////////

NewProjectDialog::NewProjectDialog(QWidget *parent, QString const &projectName, QString const &seriesName): QDialog(parent)
{
  QLabel *projectNameLabel = new QLabel(tr("Project name *"));
  QLabel *seriesNameLabel = new QLabel(tr("Series name *"));
  QLabel *seasNbrLabel = new QLabel(tr("Season number"));
  QLabel *epNbrLabel = new QLabel(tr("Episode number"));
  QLabel *epNameLabel = new QLabel(tr("Episode name (optional)"));
  QLabel *epFNameLabel = new QLabel(tr("Associated file *"));

  m_projectNameLE = new QLineEdit;
  m_seriesNameLE = new QLineEdit;
  m_seasNbrSB = new QSpinBox;
  m_epNbrSB = new QSpinBox;
  m_epNameLE = new QLineEdit;
  m_epFNameLE = new QLineEdit;

  QPushButton *browseButton = new QPushButton("...");
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);

  m_seasNbrSB->setMinimum(1);
  m_seasNbrSB->setMaximum(20);
  m_epNbrSB->setMinimum(1);
  m_epNbrSB->setMaximum(20);

  if (!projectName.isEmpty()) {
    m_projectNameLE->setText(projectName);
    m_projectNameLE->setEnabled(false);
    m_seriesNameLE->setText(seriesName);
    m_seriesNameLE->setEnabled(false);
    setWindowTitle(tr("Append Project"));
  }
  else
    setWindowTitle(tr("New Project"));

  m_epFNameLE->setEnabled(false);

  QGridLayout *gridLayout = new QGridLayout;

  gridLayout->addWidget(projectNameLabel, 0, 0);
  gridLayout->addWidget(m_projectNameLE, 0, 1);
  gridLayout->addWidget(seriesNameLabel, 1, 0);
  gridLayout->addWidget(m_seriesNameLE, 1, 1);
  gridLayout->addWidget(seasNbrLabel, 2, 0);
  gridLayout->addWidget(m_seasNbrSB, 2, 1);
  gridLayout->addWidget(epNbrLabel, 3, 0);
  gridLayout->addWidget(m_epNbrSB, 3, 1);
  gridLayout->addWidget(epNameLabel, 4, 0);
  gridLayout->addWidget(m_epNameLE, 4, 1);
  gridLayout->addWidget(epFNameLabel, 5, 0);
  gridLayout->addWidget(m_epFNameLE, 5, 1);
  gridLayout->addWidget(browseButton, 5, 2);

  QWidget *gridWidget = new QWidget;
  gridWidget->setLayout(gridLayout);

  QVBoxLayout *vLayout = new QVBoxLayout;
  vLayout->addWidget(gridWidget);
  vLayout->addWidget(buttonBox);

  setLayout(vLayout);

  connect(browseButton, SIGNAL(clicked()), this, SLOT(setMovieFName()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(checkForm()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

////////////////
// destructor //
////////////////

NewProjectDialog::~NewProjectDialog()
{
}

///////////////
// accessors //
///////////////

QString NewProjectDialog::getProjectName() const
{
  return m_projectName;
}

QString NewProjectDialog::getSeriesName() const
{
  return m_seriesName;
}

QString NewProjectDialog::getEpName() const
{
  return m_epName;
}

QString NewProjectDialog::getEpFName() const
{
  return m_epFName;
}

int NewProjectDialog::getSeasNbr() const
{
  return m_seasNbr;
}

int NewProjectDialog::getEpNbr() const
{
  return m_epNbr;
}

///////////
// slots //
///////////

void NewProjectDialog::setMovieFName()
{
  m_epFName = QFileDialog::getOpenFileName(this, tr("Open Movie"), QString(), tr("Movie Files (*.m4v *.mp4 *.mov *.avi)"));

  m_epFNameLE->setText(m_epFName);
}

void NewProjectDialog::checkForm()
{
  m_projectName = m_projectNameLE->text();
  m_seriesName = m_seriesNameLE->text();
  m_seasNbr = m_seasNbrSB->value();
  m_epNbr = m_epNbrSB->value();
  m_epName = m_epNameLE->text();
  m_epFName = m_epFNameLE->text();

  if (!m_projectName.isEmpty() && !m_seriesName.isEmpty() && !m_epFName.isEmpty())
    accept();
}
