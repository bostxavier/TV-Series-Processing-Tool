#ifndef NARRCHARTMONITOR_H
#define NARRCHARTMONITOR_H

#include <QWidget>
#include <QScrollArea>
#include <QTimer>

#include "SocialNetProcessor.h"
#include "NarrChartWidget.h"
#include "PlayerControls.h"

class NarrChartMonitor: public QWidget
{
  Q_OBJECT
  
 public:
  NarrChartMonitor(QWidget *parent = 0);

  public slots:
  void grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);
void viewNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments);
void play();
void pause();
void stop();
void setPlaybackRate(qreal rate);
void updateNarrChartWidget();

    signals:

  private:
    SocialNetProcessor *m_socialNetProcessor;
    NarrChartWidget *m_narrChartWidget;
    QScrollArea *m_scrollArea;
    PlayerControls *m_controls;
QTimer *m_timer;
qreal m_refRate;
qreal m_currRate;
int m_currScene;
int m_nbScenes;
QPair<int, int> m_sceneBound;
};

#endif
