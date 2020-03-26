#include "ResultsDialog.h"

#include <QLabel>
#include <QGridLayout>
#include <QDialogButtonBox>

ResultsDialog::ResultsDialog(qreal thresh1, qreal thresh2, qreal precision, qreal recall, qreal fScore, qreal accuracy, qreal cosine, qreal l2, qreal jaccard, qreal coverage, qreal overflow, qreal secFScore, int tp, int fp, int fn, int tn, QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle("Thresholds: " + QString::number(thresh1) + " - " + QString::number(thresh2));

  QLabel *refText = new QLabel("<b>Ref.</b>");
  QLabel *hypText = new QLabel("<b>Hyp.</b>");
  QLabel *yes1 = new QLabel("yes");
  QLabel *no1 = new QLabel("no");
  QLabel *yes2 = new QLabel("yes");
  QLabel *no2 = new QLabel("no");
  QLabel *tpLab = new QLabel(QString::number(tp));
  QLabel *fpLab = new QLabel(QString::number(fp));
  QLabel *fnLab = new QLabel(QString::number(fn));
  QLabel *tnLab = new QLabel(QString::number(tn));
  QLabel *precLab = new QLabel("<b>Precision:</b>");
  QLabel *recLab = new QLabel("<b>Recall:</b>");
  QLabel *fScLab_1 = new QLabel("<b>F-Score:</b>");
  QLabel *accLab = new QLabel("<b>Accuracy:</b>");
  QLabel *cosLab = new QLabel("<b>Cosine sim.:</b>");
  QLabel *normL2Lab = new QLabel("<b>L2 dist.:</b>");
  QLabel *indJacLab = new QLabel("<b>Jaccard index:</b>");
  QLabel *prec = new QLabel(QString::number(precision, 'g', 2));
  QLabel *rec = new QLabel(QString::number(recall, 'g', 2));
  QLabel *fSc = new QLabel("<b>" + QString::number(fScore, 'g', 2) + "</b>");
  QLabel *acc = new QLabel(QString::number(accuracy, 'g', 2));
  QLabel *cos = new QLabel("<b>" + QString::number(cosine, 'g', 2) + "</b>");
  QLabel *normL2 = new QLabel("<b>" + QString::number(l2, 'g', 2) + "</b>");
  QLabel *indJac = new QLabel("<b>" + QString::number(jaccard, 'g', 2) + "</b>");
  
  prec->setTextInteractionFlags(Qt::TextSelectableByMouse);
  rec->setTextInteractionFlags(Qt::TextSelectableByMouse);
  fSc->setTextInteractionFlags(Qt::TextSelectableByMouse);
  acc->setTextInteractionFlags(Qt::TextSelectableByMouse);
  cos->setTextInteractionFlags(Qt::TextSelectableByMouse);
  normL2->setTextInteractionFlags(Qt::TextSelectableByMouse);
  indJac->setTextInteractionFlags(Qt::TextSelectableByMouse);

  QGridLayout *layout = new QGridLayout;
  layout->setColumnMinimumWidth(2, 50);
  layout->setColumnMinimumWidth(3, 50);

  if (tp >= 0 || fp >= 0  || fn >= 0 || tn >= 0) {
    layout->addWidget(refText, 0, 2, 1, 2, Qt::AlignHCenter);
    layout->addWidget(hypText, 2, 0, 2, 1, Qt::AlignVCenter);
    layout->addWidget(yes1, 1, 2, Qt::AlignHCenter);
    layout->addWidget(no1, 1, 3, Qt::AlignHCenter);
    layout->addWidget(yes2, 2, 1, Qt::AlignHCenter);
    layout->addWidget(no2, 3, 1, Qt::AlignHCenter);
    layout->addWidget(tpLab, 2, 2, Qt::AlignHCenter);
    layout->addWidget(fpLab, 2, 3, Qt::AlignHCenter);
    layout->addWidget(fnLab, 3, 2, Qt::AlignHCenter);
    layout->addWidget(tnLab, 3, 3, Qt::AlignHCenter);
  }

  layout->addWidget(precLab, 4, 0, 1, 2);
  layout->addWidget(prec, 4, 2, 1, 2, Qt::AlignRight);
  layout->addWidget(recLab, 5, 0, 1, 2);
  layout->addWidget(rec, 5, 2, 1, 2, Qt::AlignRight);
  layout->addWidget(fScLab_1, 6, 0, 1, 2);
  layout->addWidget(fSc, 6, 2, 1, 2, Qt::AlignRight);
  layout->addWidget(accLab, 7, 0, 1, 2);
  layout->addWidget(acc, 7, 2, 1, 2, Qt::AlignRight);

  
  QLabel *covLab = new QLabel("<b>Coverage:</b>");
  QLabel *overLab = new QLabel("<b>Overflow:</b>");
  QLabel *fScLab_2 = new QLabel("<b>F-Score:</b>");
  QLabel *cov = new QLabel(QString::number(coverage, 'g', 2));
  QLabel *over = new QLabel(QString::number(overflow, 'g', 2));
  QLabel *secFSc = new QLabel("<b>" + QString::number(secFScore, 'g', 2) + "</b>");

  cov->setTextInteractionFlags(Qt::TextSelectableByMouse);

  if (cosine >= 0) {
    layout->addWidget(cosLab, 8, 0, 1, 2);
    layout->addWidget(cos, 8, 2, 1, 2, Qt::AlignRight);
  }

  if (l2 >= 0) {
    layout->addWidget(normL2Lab, 9, 0, 1, 2);
    layout->addWidget(normL2, 9, 2, 1, 2, Qt::AlignRight);
  }

  if (jaccard >= 0) {
    layout->addWidget(indJacLab, 10, 0, 1, 2);
    layout->addWidget(indJac, 10, 2, 1, 2, Qt::AlignRight);
  }

  if (coverage >= 0) {
    layout->addWidget(covLab, 11, 0, 1, 2);
    layout->addWidget(cov, 11, 2, 1, 2, Qt::AlignRight);
  }
  
  if (overflow >= 0) {
    layout->addWidget(overLab, 12, 0, 1, 2);
    layout->addWidget(over, 12, 2, 1, 2, Qt::AlignRight);
  }

  if (secFScore >= 0) {
    layout->addWidget(fScLab_2, 13, 0, 1, 2);
    layout->addWidget(secFSc, 13, 2, 1, 2, Qt::AlignRight);
  }

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  layout->addWidget(buttonBox, 14, 0, 1, 4, Qt::AlignHCenter);

  setLayout(layout);

  connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
}
