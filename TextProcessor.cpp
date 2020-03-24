#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QtCore/qmath.h>

#include "TextProcessor.h"

using namespace arma;

TextProcessor::TextProcessor(QObject *parent)
  : QObject(parent)
{
  QFile stopListFile("nlpProcessing/longStopList.csv");

  if (!stopListFile.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QTextStream in(&stopListFile);

  while (!in.atEnd()) {

    QString line = in.readLine();
    QString processedLine = preProcess(line);
    QStringList unigrams = extractNGrams(processedLine);

    for (int i(0); i < unigrams.size(); i++)
      if (!m_stopList.contains(unigrams[i]))
	m_stopList.push_back(unigrams[i]);
  }
}

void TextProcessor::setIndex(const QList<QString> &subText)
{
  QString processedText;
  QStringList nGrams;
  m_index.resize(subText.size());

  for (int i(0); i < subText.size(); i++) {

    processedText = preProcess(subText[i]);

    if (!processedText.isEmpty()) {

      nGrams = extractNGrams(processedText);

      // number of occurrences of each ngram in current subtitle
      QMap<QString, int> nOcc;

      for (int j(0); j < nGrams.size(); j++)
	nOcc[nGrams[j]]++;

      // updating inverted index
      QMap<QString, int>::const_iterator it =  nOcc.begin();

      while (it != nOcc.end()) {

	QString nGram = it.key();

	if (!m_stopList.contains(nGram))
	  m_index[i].push_back(QPair<QString, int>(nGram, it.value()));

	it++;
      }
    }
  }
}

QString TextProcessor::preProcess(QString subText)
{
  // processing abbeviated negation
  subText.replace(QRegularExpression("can't"), "can not");
  subText.replace(QRegularExpression("won't"), "will not");
  subText.replace(QRegularExpression("n't"), " not");

  // processing single quotes
  subText.replace(QRegularExpression("'"), " ");

  // removing html tags
  QRegularExpression htmlTag("<i>|</i>|<br />");
  subText.replace(htmlTag, " ");

  // removing hyphens
  subText.replace(QRegularExpression("-"), " ");

  // removing punctuation characters
  QRegularExpression punct("[,.?!\"“_*]");
  subText.remove(punct);

  /*********************/
  /* processing spaces */
  /*********************/

  // consecutive spaces
  subText.replace(QRegularExpression("\\s+"), " ");

  // spaces at the beginning or at the end of the string
  subText.remove(QRegularExpression("^\\s+|\\s+$"));

  return subText.toLower();
}

QStringList TextProcessor::extractNGrams(const QString &processedText)
{
  QStringList nGrams;

  nGrams = processedText.split(" ");

  return nGrams;
}

mat TextProcessor::computeLSULexSim(const QList<QPair<int, int> > &lsuUtterBound)
{
  arma::mat S;
  S.zeros(lsuUtterBound.size(), lsuUtterBound.size());
  QVector<QList<QPair<QString, int> > > lsuIndex;
  QMap<QString, qreal> lsuIdf;
  QList<QList<int> > subLSU;

  // enumerating all utterances covered by each LSU
  for (int i(0); i < lsuUtterBound.size(); i++) {

    QList<int> uttIndices;

    for (int j(lsuUtterBound[i].first); j <= lsuUtterBound[i].second; j++)
      uttIndices.push_back(j);

    subLSU.push_back(uttIndices);
  }

  // generating inverted index term <-> LSU as a document
  lsuIndex = genDocIndex(subLSU);

  // compute LSU Inverse Document Frequency
  lsuIdf = computeIdf(lsuIndex);
  
  // loop twice over LSUs to compute cosine similarity
  for (int i(0); i < lsuIndex.size() - 1; i++) {
    for (int j(i+1); j < lsuIndex.size(); j++)
      if (lsuUtterBound[i].first != -1 && lsuUtterBound[j].first != -1) {
      
	/*
	qDebug() << lsuUtterBound[i].first << "<->" << lsuUtterBound[i].second;
	qDebug() << lsuUtterBound[j].first << "<->" << lsuUtterBound[j].second;
	qDebug();
	*/
	S(i, j) = computeSimCos(lsuIndex[i], lsuIndex[j], lsuIdf);
    }
  }

  return S;
}

QVector<QList<QPair<QString, int> > > TextProcessor::genDocIndex(const QList<QList<int> > &subDoc)
{
  QVector<QList<QPair<QString, int> > > docIndex;
  docIndex.resize(subDoc.size());

  // looping over utterances contained in each document
  for (int i(0); i < subDoc.size(); i++) {

    QList<int> subIndices = subDoc[i];

      for (int j(0); j < subIndices.size(); j++) {

	// document contains speech: update document/term matrix
	if (subIndices[j] != -1) {

	  QList<QPair<QString, int> > termFreq = m_index[subIndices[j]];

	  for (int k(0); k < termFreq.size(); k++) {

	    if (!containsFirst(docIndex[i], termFreq[k].first))
	      docIndex[i].push_back(termFreq[k]);
	    else
	      updateCount(docIndex[i], termFreq[k].first, termFreq[k].second);
	  }
	}
      }
  }
  
  return docIndex;
}

QMap<QString, qreal> TextProcessor::computeIdf(const QVector<QList<QPair<QString, int> > > &docIndex)
{
  QMap<QString, qreal> idf;
  QMap<QString, int> df;

  // looping over documents to estimate document frequency
  for (int i(0); i < docIndex.size(); i++)
    for (int j(0); j < docIndex[i].size(); j++)
      df[docIndex[i][j].first]++;

  // looping over terms to estimate inverse document frequency
  QMap<QString, int>::const_iterator it = df.begin();
  while (it != df.end()) {

    QString nGram = it.key();

    idf[nGram] = qLn(static_cast<qreal>(docIndex.size()) / df[nGram]);

    it++;
  }

  return idf;
}

qreal TextProcessor::computeSimCos(const QList<QPair<QString, int> > &v1, const QList<QPair<QString, int> > &v2, const QMap<QString, qreal> &idf)
{
  qreal cos(0.0);
  qreal l1(0.0);
  qreal l2(0.0);
  int n1(v1.size());
  int n2(v2.size());
  QMap<qreal, QString> ctb;

  /*
  // display vector content for debugging purpose
  qDebug() << "V1 -->";
  for (int i(0); i < n1; i++)
    qDebug() << v1[i].first;

  qDebug();

  qDebug() << "V2 -->";
  for (int i(0); i < n2; i++)
    qDebug() << v2[i].first;

  qDebug();
  */

  // compute lengths of each vector
  for (int i(0); i < n1; i++)
    l1 += qPow(v1[i].second * idf[v1[i].first], 2);

  for (int i(0); i < n2; i++)
    l2 += qPow(v2[i].second * idf[v2[i].first], 2);

  l1 = qSqrt(l1);
  l2 = qSqrt(l2);

  // compute scalar product
  if (n1 < n2)
    
    for (int i(0); i < n1; i++) {

      QString nGram = v1[i].first;
      int tf = v1[i].second;

      if (containsFirst(v2, nGram)) {
	qreal prod = prodComponent(v2, nGram, tf) * qPow(idf[nGram], 2);
	cos += prod;
	ctb[prod] = nGram;
      }
    }

  else

    for (int i(0); i < n2; i++) {
      
      QString nGram = v2[i].first;
      int tf = v2[i].second;

      if (containsFirst(v1, nGram)) {
	qreal prod = prodComponent(v1, nGram, tf) * qPow(idf[nGram], 2);
	cos += prod;
	ctb[prod] = nGram;
      }
    }

  if (l1 != 0 && l2 != 0)
    cos /= l1 * l2;
  else
    cos = 0.0;

  // display contributors
  /*
  QMap<qreal, QString>::const_iterator it = ctb.begin();

  while (it != ctb.end()) {

    qDebug() << it.value() << "\t" << it.key();

    it++;
  }

  qDebug() << endl << "***" << cos << "***" << endl;
  */

  return cos;
}

bool TextProcessor::containsFirst(const QList<QPair<QString, int> > &list, const QString &s)
{
   for (int i(0); i < list.size(); i++)
    if (list[i].first == s)
      return true;

  return false;
}

void TextProcessor::updateCount(QList<QPair<QString, int> > &list, const QString &s, int value)
{
  for (int i(0); i < list.size(); i++)

     if (list[i].first == s) {
       list[i].second += value;
       return;
     }
}

qreal TextProcessor::prodComponent(const QList<QPair<QString, int> > &list, const QString &s, int value)
{
  for (int i(0); i < list.size(); i++)

     if (list[i].first == s)
       return list[i].second * value;

  return 0.0;
}
