#ifndef RESULTSDIALOG_H
#define RESULTSDIALOG_H

#include <QDialog>

class ResultsDialog: public QDialog
{
 public:
  ResultsDialog(qreal thresh1, qreal thresh2, qreal precision, qreal recall, qreal fScore, qreal accuracy, qreal cosine = -1.0, qreal l2 = -1.0, qreal jaccard = -1.0, qreal coverage = -1.0, qreal overflow = -1.0, qreal secFScore = -1.0, int tp = -1, int fp = -1, int fn = -1, int tn = -1, QWidget *parent = 0);

 private:
};

#endif
