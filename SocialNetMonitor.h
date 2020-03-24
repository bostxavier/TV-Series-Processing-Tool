#ifndef SOCIALNETMONITOR_H
#define SOCIALNETMONITOR_H

#include <QWidget>
#include <QSlider>
#include <QCheckBox>
#include <QTimer>

#include "SocialNetProcessor.h"
#include "SocialNetWidget.h"
#include "PlayerControls.h"

class SocialNetMonitor: public QWidget
{
  Q_OBJECT

public:
  SocialNetMonitor(QWidget *parent = 0);

public slots:
  void viewSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments);
  void updateSpkView(int iScene);
  void activCoOccurr();
  void activInteract();
  void setDirected(bool checked);
  void setWeighted(bool checked);
  void exportGraphToFile();
  void setLabelDisplay(bool checked);
  void activTwoDim();
  void activThreeDim();
  void activVertexFilter();
  void activEdgeFilter();
  void setFilterWeight(int value);
  void adjustDirect(bool checked);
  void adjustWeight(bool checked);
  void activNbWeight(bool checked);
  void activDurWeight(bool checked);
  void play();
  void pause();
  void stop();
  void setPlaybackRate(qreal rate);
  void resetPlayer();

 signals:
  void durationChanged(qint64 duration);

private:
  void updateViews();

  PlayerControls *m_controls;
  SocialNetProcessor *m_socialNetProcessor;
  SocialNetWidget *m_socialNetWidget;
  SocialNetProcessor::GraphSource m_src;
  QSlider *m_filterWeight;
  bool m_vertexFilter;
  QCheckBox *m_direct;
  QCheckBox *m_weight;
  QTimer *m_timer;
  qreal m_refRate;
  qreal m_currRate;
  int m_defaultFilter;
};

#endif
