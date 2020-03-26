#ifndef SOCIALNETMONITOR_H
#define SOCIALNETMONITOR_H

#include <igraph.h>

#include <QGLWidget>

class SocialNetWidget: public QGLWidget
{
  Q_OBJECT

    public:
  SocialNetWidget(int labelHeight = 10, QWidget *parent = 0);

  public slots:
  void speakersRetrieved(const igraph_t &graph);
  void positionChanged(qint64 position);

 protected:
  void paintEvent(QPaintEvent *event);

  private:
  int m_labelHeight;
  int m_position;
};

#endif
