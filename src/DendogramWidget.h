#ifndef DENDOGRAMWIDGET_H
#define DENDOGRAMWIDGET_H

#include <QWidget>

#include <armadillo>

#include "Dendogram.h"

class DendogramWidget: public QWidget
{
  Q_OBJECT
  
 public:
  DendogramWidget(const QString &speaker, Dendogram *dendogram, const QList<arma::vec> &instances, const QList<QPair<QString, QString> > &vecComponents, QWidget *parent = 0);

  public slots:
    
    signals:

 protected:
  void paintEvent(QPaintEvent * event);
  void mouseMoveEvent(QMouseEvent * event);

  QSize minimumSizeHint() const;
  QSize sizeHint() const;

  QPoint m_mousePos;

 private:
  QList<QPair<qreal, QPair<QString, QString> > > getSocialEnv(const arma::vec &U);
  QColor getRGBColor(const QColor &refColor, qreal weight);

  QString m_speaker;
  Dendogram *m_dendogram;
  QList<arma::vec> m_instances;
  QList<QPair<QString, QString> > m_vecComponents;

  QList<int> m_selectedLabels;
  QList<QPair<qreal, QPair<QString, QString> > > m_meanSocialEnv;
  int m_iMed;
  QList<QPair<qreal, QPair<QString, QString> > > m_medSocialEnv;
};

#endif
