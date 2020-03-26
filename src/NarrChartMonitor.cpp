#include <QGridLayout>
#include <QCheckBox>
#include <QScrollBar>

#include "NarrChartMonitor.h"

NarrChartMonitor::NarrChartMonitor(QWidget *parent)
  : QWidget(parent),
    m_refRate(40),
    m_currRate(m_refRate),
    m_currScene(-1)
{
  m_socialNetProcessor = new SocialNetProcessor;
  m_narrChartWidget = new NarrChartWidget;

  m_scrollArea = new QScrollArea;
  m_scrollArea->setWidget(m_narrChartWidget);
  m_scrollArea->setMinimumSize(1400, 500);

  QCheckBox *displayLines = new QCheckBox(tr("Display lines"));
  QCheckBox *displayArcs = new QCheckBox(tr("Display arcs"));
  m_controls = new PlayerControls(false);
  m_timer = new QTimer(this);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(m_scrollArea, 0, 0, 1, 10);
  layout->addWidget(displayLines, 1, 0);
  layout->addWidget(displayArcs, 1, 1);
  layout->addWidget(m_controls, 1, 2);

  setLayout(layout);

  connect(displayLines, SIGNAL(clicked(bool)), m_narrChartWidget, SLOT(displayLines(bool)));
  connect(displayArcs, SIGNAL(clicked(bool)), m_narrChartWidget, SLOT(displayArcs(bool)));

  connect(m_controls, SIGNAL(play()), this, SLOT(play()));
  connect(m_controls, SIGNAL(pause()), this, SLOT(pause()));
  connect(m_controls, SIGNAL(stop()), this, SLOT(stop()));
  connect(m_controls, SIGNAL(playbackRateChanged(qreal)), this, SLOT(setPlaybackRate(qreal)));

  connect(m_timer, SIGNAL(timeout()), this, SLOT(updateNarrChartWidget()));
}

///////////
// slots //
///////////

void NarrChartMonitor::viewNarrChart(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  m_socialNetProcessor->setSceneSpeechSegments(sceneSpeechSegments);
  m_socialNetProcessor->setGraph(SocialNetProcessor::Interact, false);
  m_socialNetProcessor->setNarrChart();
  
  QVector<QMap<QString, QMap<QString, qreal> > > snapshots = m_socialNetProcessor->getNetworkViews();
  QMap<QString, QVector<QPoint> > narrChart = m_socialNetProcessor->getNarrChart();
  QVector<QPair<int, int> > sceneRefs = m_socialNetProcessor->getSceneRefs();

  m_narrChartWidget->setSnapshots(snapshots);
  m_narrChartWidget->setNarrChart(narrChart);
  m_narrChartWidget->setSceneRefs(sceneRefs);

  m_sceneBound = m_narrChartWidget->getSceneBound();
  m_currScene = m_sceneBound.first;

  m_narrChartWidget->update();
  show();
}

void NarrChartMonitor::grabSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots)
{
  m_narrChartWidget->setSnapshots(snapshots);
}

void NarrChartMonitor::play()
{
  m_timer->start(m_currRate);
}

void NarrChartMonitor::pause()
{
  m_timer->stop();
}
 
void NarrChartMonitor::stop()
{
  m_timer->stop();
  m_currScene = m_sceneBound.first;
}

void NarrChartMonitor::setPlaybackRate(qreal rate)
{
  m_currRate = m_refRate / rate;
  m_timer->stop();
}

void NarrChartMonitor::updateNarrChartWidget()
{
  int x = m_narrChartWidget->getXCoord(m_currScene);
  m_scrollArea->horizontalScrollBar()->setValue(x);

  m_currScene++;

  if (m_currScene > m_sceneBound.second)
    m_currScene = m_sceneBound.first;
}
