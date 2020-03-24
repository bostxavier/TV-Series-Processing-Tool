#include <QGroupBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QFileDialog>

#include <QDebug>

#include "SocialNetMonitor.h"

SocialNetMonitor::SocialNetMonitor(QWidget *parent)
  : QWidget(parent),
    m_src(SocialNetProcessor::Interact),
    m_vertexFilter(true),
    m_refRate(40),
    m_currRate(m_refRate),
    m_defaultFilter(15)
{
  m_socialNetProcessor = new SocialNetProcessor;
  m_socialNetWidget = new SocialNetWidget;

  QGroupBox *srcBox = new QGroupBox("Graph type:");
  QRadioButton *coOccurr = new QRadioButton("Speaker oriented");
  QRadioButton *interact = new QRadioButton("Utterance oriented");
  interact->setChecked(true);
  QGridLayout *srcLayout = new QGridLayout;
  srcLayout->addWidget(coOccurr, 0, 0);
  srcLayout->addWidget(interact, 1, 0);
  srcBox->setLayout(srcLayout);
  
  m_direct = new QCheckBox(tr("Direct links"));
  m_direct->setChecked(false);

  m_weight = new QCheckBox(tr("Weight links"));
  m_weight->setChecked(true);

  QGroupBox *weightBox = new QGroupBox("Weighting source:");
  QRadioButton *nbInteract = new QRadioButton("Nb. of interactions");
  QRadioButton *durInteract = new QRadioButton("Duration of interactions");
  durInteract->setChecked(true);
  QGridLayout *weightLayout = new QGridLayout;
  weightLayout->addWidget(nbInteract, 0, 0);
  weightLayout->addWidget(durInteract, 1, 0);
  weightBox->setLayout(weightLayout);

  QPushButton *exportGraph = new QPushButton(tr("&Export graph to file..."));

  QGroupBox *layoutBox = new QGroupBox("Layout:");
  QRadioButton *twoDim = new QRadioButton("2D");
  QRadioButton *threeDim = new QRadioButton("3D");
  twoDim->setChecked(true);
  QGridLayout *layoutLayout = new QGridLayout;
  layoutLayout->addWidget(twoDim, 0, 0);
  layoutLayout->addWidget(threeDim, 1, 0);
  layoutBox->setLayout(layoutLayout);

  QGroupBox *filterBox = new QGroupBox("Filter on:");
  QRadioButton *vertexDegree = new QRadioButton("Speaker degree");
  QRadioButton *edgeWeight = new QRadioButton("Interaction weight");
  vertexDegree->setChecked(true);
  QGridLayout *filterLayout = new QGridLayout;
  filterLayout->addWidget(vertexDegree, 0, 0);
  filterLayout->addWidget(edgeWeight, 1, 0);
  filterBox->setLayout(filterLayout);
  m_filterWeight = new QSlider(Qt::Horizontal);
  m_filterWeight->setMinimum(0);
  m_filterWeight->setValue(m_defaultFilter);
  m_filterWeight->setMaximum(100);

  QCheckBox *displayLabels = new QCheckBox(tr("Display node labels"));
  QCheckBox *displayEdges = new QCheckBox(tr("Display edges"));

  QFrame *line_1 = new QFrame;
  line_1->setFrameShape(QFrame::HLine);
  line_1->setFrameShadow(QFrame::Sunken);

  m_controls = new PlayerControls(false);

  QFrame *line_2 = new QFrame;
  line_2->setFrameShape(QFrame::HLine);
  line_2->setFrameShadow(QFrame::Sunken);

  QFrame *line_3 = new QFrame;
  line_3->setFrameShape(QFrame::HLine);
  line_3->setFrameShadow(QFrame::Sunken);

  m_timer = new QTimer(this);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(m_socialNetWidget, 0, 0, 10, 10);
  // layout->addWidget(srcBox, 0, 10);
  // layout->addWidget(m_direct, 1, 10);
  // layout->addWidget(m_weight, 2, 10);
  // layout->addWidget(weightBox, 3, 10);
  layout->addWidget(m_controls, 0, 10, 1, 2);
  layout->addWidget(line_1, 1, 10, 1, 2);
  layout->addWidget(layoutBox, 2, 10);
  layout->addWidget(displayLabels, 3, 10);
  layout->addWidget(displayEdges, 4, 10);
  layout->addWidget(line_3, 5, 10, 1, 2);
  layout->addWidget(filterBox, 6, 10);
  layout->addWidget(m_filterWeight, 7, 10, 1, 2);
  layout->addWidget(line_2, 8, 10, 1, 2);
  layout->addWidget(exportGraph, 9, 10, 1, 2, Qt::AlignHCenter);
  
  setLayout(layout);

  // connecting signals to corresponding slots
  connect(coOccurr, SIGNAL(clicked()), this, SLOT(activCoOccurr()));
  connect(interact, SIGNAL(clicked()), this, SLOT(activInteract()));
  connect(m_direct, SIGNAL(clicked(bool)), this, SLOT(setDirected(bool)));
  connect(m_direct, SIGNAL(clicked(bool)), this, SLOT(adjustWeight(bool)));
  connect(m_weight, SIGNAL(clicked(bool)), this, SLOT(setWeighted(bool)));
  connect(m_weight, SIGNAL(clicked(bool)), this, SLOT(adjustDirect(bool)));
  connect(nbInteract, SIGNAL(clicked(bool)), this, SLOT(activNbWeight(bool)));
  connect(durInteract, SIGNAL(clicked(bool)), this, SLOT(activDurWeight(bool)));
  connect(exportGraph, SIGNAL(clicked()), this, SLOT(exportGraphToFile()));
  connect(displayLabels, SIGNAL(clicked(bool)), this, SLOT(setLabelDisplay(bool)));

  connect(m_weight, SIGNAL(clicked(bool)), weightBox, SLOT(setEnabled(bool)));
  connect(interact, SIGNAL(clicked(bool)), m_direct, SLOT(setEnabled(bool)));
  connect(coOccurr, SIGNAL(clicked(bool)), m_direct, SLOT(setDisabled(bool)));
  connect(twoDim, SIGNAL(clicked()), this, SLOT(activTwoDim()));
  connect(threeDim, SIGNAL(clicked()), this, SLOT(activThreeDim()));
  connect(vertexDegree, SIGNAL(clicked()), this, SLOT(activVertexFilter()));
  connect(edgeWeight, SIGNAL(clicked()), this, SLOT(activEdgeFilter()));
  connect(m_filterWeight, SIGNAL(valueChanged(int)), this, SLOT(setFilterWeight(int)));

  connect(m_controls, SIGNAL(play()), this, SLOT(play()));
  connect(m_controls, SIGNAL(pause()), this, SLOT(pause()));
  connect(m_controls, SIGNAL(stop()), this, SLOT(stop()));
  connect(m_controls, SIGNAL(playbackRateChanged(qreal)), this, SLOT(setPlaybackRate(qreal)));
  connect(this, SIGNAL(durationChanged(qint64)), m_controls, SLOT(durationChanged(qint64)));
  connect(m_socialNetWidget, SIGNAL(positionChanged(qint64)), m_controls, SLOT(positionChanged(qint64)));
  connect(m_controls, SIGNAL(positionManuallyChanged(qint64)), m_socialNetWidget, SLOT(setSceneIndex(qint64)));
  connect(m_controls, SIGNAL(newPosition(qint64)), m_socialNetWidget, SLOT(setSceneIndex(qint64)));
  
  connect(m_timer, SIGNAL(timeout()), m_socialNetWidget, SLOT(paintNextScene()));

  connect(m_socialNetWidget, SIGNAL(resetPlayer()), this, SLOT(resetPlayer()));
}

void SocialNetMonitor::viewSpkNet(QList<QList<SpeechSegment *> > sceneSpeechSegments)
{
  m_socialNetProcessor->setSceneSpeechSegments(sceneSpeechSegments);
  m_socialNetProcessor->setGraph(m_src);

  // set widget vertices and edges
  QVector<QList<Vertex> > verticesViews = m_socialNetProcessor->getVerticesViews();
  m_socialNetWidget->setVerticesViews(verticesViews);
  QVector<QList<Edge> > edgesViews = m_socialNetProcessor->getEdgesViews();
  m_socialNetWidget->setEdgesViews(edgesViews);
  QVector<QPair<int, int> > sceneRefs = m_socialNetProcessor->getSceneRefs();
  m_socialNetWidget->setSceneRefs(sceneRefs);

  // set maximum position
  emit durationChanged(verticesViews.size() - 1);

  // filter vertices
  setFilterWeight(m_defaultFilter);

  updateViews();
}

void SocialNetMonitor::updateSpkView(int iScene)
{
  m_socialNetWidget->updateNetView(iScene);
  updateViews();
}

void SocialNetMonitor::updateViews()
{
  // filter widget vertices/edges
  if (m_vertexFilter)
    m_socialNetWidget->selectVertices();
  else
    m_socialNetWidget->selectEdges();

  m_socialNetWidget->updateWidget();
}

///////////
// slots //
///////////

void SocialNetMonitor::activCoOccurr()
{
  m_src = SocialNetProcessor::CoOccurr;
  m_socialNetProcessor->setGraph(m_src);
  updateViews();
}

void SocialNetMonitor::activInteract()
{
  m_src = SocialNetProcessor::Interact;
  m_socialNetProcessor->setGraph(m_src);
  updateViews();
}

void SocialNetMonitor::setDirected(bool checked)
{
  m_socialNetProcessor->setDirected(checked);
  m_socialNetProcessor->updateGraph(m_src);
  updateViews();
}

void SocialNetMonitor::setWeighted(bool checked)
{
  m_socialNetProcessor->setWeighted(checked);
  m_socialNetProcessor->updateGraph(m_src);
  updateViews();
}

void SocialNetMonitor::exportGraphToFile()
{
  QString fName = QFileDialog::getSaveFileName(this, tr("Export speakers network"), tr("tools/sna/graph_files"), tr("GrapML file (*.graphml)"));
  
  // if (!fName.isEmpty())
  // m_socialNetProcessor->exportGraphToFile(fName + ".graphml");
}

void SocialNetMonitor::setLabelDisplay(bool checked)
{
  m_socialNetWidget->setLabelDisplay(checked);
}

void SocialNetMonitor::activTwoDim()
{
  m_socialNetWidget->setTwoDim(true);
}
 
void SocialNetMonitor::activThreeDim()
{
  m_socialNetWidget->setTwoDim(false);
}

void SocialNetMonitor::activVertexFilter()
{
  m_vertexFilter = true;
  m_socialNetWidget->selectVertices();
}

void SocialNetMonitor::activEdgeFilter()
{
  m_vertexFilter = false;
  m_socialNetWidget->selectEdges();
}

void SocialNetMonitor::setFilterWeight(int value)
{
  qreal filterWeight = value / 100.0;

  m_socialNetWidget->setFilterWeight(filterWeight);

  if (m_vertexFilter)
    m_socialNetWidget->selectVertices();
  else
    m_socialNetWidget->selectEdges();
}

void SocialNetMonitor::adjustDirect(bool checked)
{
  if (!checked) {
    m_direct->setChecked(checked);
    setDirected(checked);
  }
}

void SocialNetMonitor::adjustWeight(bool checked)
{
  if (checked) {
    m_weight->setChecked(checked);
    setWeighted(checked);
  }
}

void SocialNetMonitor::activNbWeight(bool checked)
{
  m_socialNetProcessor->setNbWeight(checked);
  m_socialNetProcessor->updateGraph(m_src);
  updateViews();
}

void SocialNetMonitor::activDurWeight(bool checked)
{
  m_socialNetProcessor->setNbWeight(!checked);
  m_socialNetProcessor->updateGraph(m_src);
  updateViews();
}

void SocialNetMonitor::play()
{
  m_timer->start(m_currRate);
}

void SocialNetMonitor::pause()
{
  m_timer->stop();
}
 
void SocialNetMonitor::stop()
{
  m_timer->stop();
  m_socialNetWidget->updateNetView(0);
  updateViews();
}

void SocialNetMonitor::setPlaybackRate(qreal rate)
{
  m_currRate = m_refRate / rate;
  // m_controls->stopClicked();
  m_timer->stop();
}

void SocialNetMonitor::resetPlayer()
{
  m_timer->stop();
  m_controls->stopClicked();
  m_socialNetWidget->updateNetView(0);
}
