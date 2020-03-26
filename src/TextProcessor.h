#ifndef TEXTPROCESSOR_H
#define TEXTPROCESSOR_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <armadillo>

class TextProcessor: public QObject
{
  Q_OBJECT

 public:
  TextProcessor(QObject *parent = 0);
  QString preProcess(QString subText);

  void setIndex(const QList<QString> &subText);
  arma::mat computeLSULexSim(const QList<QPair<int, int> > &lsuUtterBound);

  public slots:
    
    private:
  QStringList extractNGrams(const QString &processedText);
  QVector<QList<QPair<QString, int> > > genDocIndex(const QList<QList<int> > &subDoc);
  QMap<QString, qreal> computeIdf(const QVector<QList<QPair<QString, int> > > &docIndex);
  qreal computeSimCos(const QList<QPair<QString, int> > &v1, const QList<QPair<QString, int> > &v2, const QMap<QString, qreal> &idf);
  bool containsFirst(const QList<QPair<QString, int> > &list, const QString &s);
  void updateCount(QList<QPair<QString, int> > &list, const QString &s, int value);
  qreal prodComponent(const QList<QPair<QString, int> > &list, const QString &s, int value);

  QVector<QList<QPair<QString, int> > > m_index;
  QList<QString> m_stopList;
};

#endif
