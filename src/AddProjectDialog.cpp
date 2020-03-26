#include <QWidget>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>

#include "AddProjectDialog.h"

/////////////////
// constructor //
/////////////////

AddProjectDialog::AddProjectDialog(QString const &projectName, QString const &seriesName, QWidget *parent)
  : QDialog(parent)
{
  QLabel *projectNameLabel = new QLabel(tr("New project name *"));
  QLabel *seriesNameLabel = new QLabel(tr("Series name *"));
  QLabel *secProjectFNameLabel = new QLabel(tr("Project file to include *"));

  m_projectNameLE = new QLineEdit;
  QLineEdit *seriesNameLE = new QLineEdit;
  m_secProjectFNameLE = new QLineEdit;

  QPushButton *browseButton = new QPushButton("...");
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);

  m_projectNameLE->setText(projectName);
  seriesNameLE->setText(seriesName);
  seriesNameLE->setEnabled(false);
  setWindowTitle(tr("Add Project"));

  m_secProjectFNameLE->setEnabled(false);

  QGridLayout *gridLayout = new QGridLayout;

  gridLayout->addWidget(projectNameLabel, 0, 0);
  gridLayout->addWidget(m_projectNameLE, 0, 1);
  gridLayout->addWidget(seriesNameLabel, 1, 0);
  gridLayout->addWidget(seriesNameLE, 1, 1);
  gridLayout->addWidget(secProjectFNameLabel, 2, 0);
  gridLayout->addWidget(m_secProjectFNameLE, 2, 1);
  gridLayout->addWidget(browseButton, 2, 2);

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

AddProjectDialog::~AddProjectDialog()
{
}

///////////////
// accessors //
///////////////

QString AddProjectDialog::getProjectName() const
{
  return m_projectName;
}

QString AddProjectDialog::getSecProjectFName() const
{
  return m_secProjectFName;
}

///////////
// slots //
///////////

void AddProjectDialog::setMovieFName()
{
  m_secProjectFName = QFileDialog::getOpenFileName(this, tr("Open Project File"), QString(), tr("Project Files (*.json)"));

  m_secProjectFNameLE->setText(m_secProjectFName);
}

void AddProjectDialog::checkForm()
{
  m_projectName = m_projectNameLE->text();
  m_secProjectFName = m_secProjectFNameLE->text();

  if (!m_projectName.isEmpty() && !m_secProjectFName.isEmpty())
    accept();
}
