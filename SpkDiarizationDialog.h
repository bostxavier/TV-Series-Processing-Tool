#ifndef SPKDIARIZATIONDIALOG_H
#define SPKDIARIZATIONDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>

#include "UtteranceTree.h"

class SpkDiarizationDialog: public QDialog
{
  Q_OBJECT

 public:
  enum Method {
    HC, Ref
  };

  enum Modality {
    Audio, Video, Mixed
  };

  SpkDiarizationDialog(const QString &title, bool local = true, bool view = false, QWidget *parent = 0);
  QString getSpeakersFName() const;
  UtteranceTree::DistType getDist() const;
  UtteranceTree::AgrCrit getAgrCrit() const;
  UtteranceTree::PartMeth getPartMeth() const;
  Method getMethod() const;
  bool getNorm() const;
  bool getWeight() const;
  bool getSigma() const;
  bool getUbm() const;

  public slots:
    void activL2();
    void activMahal();
    void activSigma();
    void activW();
    void activMin();
    void activMax();
    void activMean();
    void activWard();
    void activSilhouette();
    void activBipartition();
    void activHier();
    void activRefSpk();

 private:
    QString m_speakersFName;
    UtteranceTree::DistType m_dist;
    UtteranceTree::AgrCrit m_agrCrit;
    UtteranceTree::PartMeth m_partMeth;
    Method m_method;
    QCheckBox *m_ubm;
    QCheckBox *m_norm;
    QCheckBox *m_weight;
    QRadioButton *m_l2;
    QRadioButton *m_mahal;
    bool m_sigma;
    bool m_local;
};

#endif
