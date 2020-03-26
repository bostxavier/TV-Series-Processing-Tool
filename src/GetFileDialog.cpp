#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QFileDialog>

#include "GetFileDialog.h"

GetFileDialog::GetFileDialog(const QString &openFileMsg, const QString &fileType, QWidget *parent)
  : QDialog(parent),
    m_fName(QString()),
    m_openFileMsg(openFileMsg),
    m_fileType(fileType)
{
  QLabel *fNameLabel = new QLabel(openFileMsg + " *");
  m_fNameLE = new QLineEdit;

  m_fNameLE->setEnabled(false);

  QPushButton *browseButton = new QPushButton("...");
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     | QDialogButtonBox::Cancel);

  QGridLayout *layout = new QGridLayout;

  layout->addWidget(fNameLabel, 0, 0);
  layout->addWidget(m_fNameLE, 0, 1);
  layout->addWidget(browseButton, 0, 2);

  layout->addWidget(buttonBox, 1, 0, 1, 3);

  setLayout(layout);

  connect(browseButton, SIGNAL(clicked()), this, SLOT(setFName()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(checkForm()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

///////////////
// accessors //
///////////////

QString GetFileDialog::getFName() const
{
  return m_fName;
}

///////////
// slots //
///////////

void GetFileDialog::setFName()
{
  m_fName = QFileDialog::getOpenFileName(this, "Open " + m_openFileMsg, QString(), m_fileType);
  m_fNameLE->setText(m_fName);
}

void GetFileDialog::checkForm()
{
  m_fName = m_fNameLE->text();

  if (!m_fName.isEmpty())
    accept();
}
