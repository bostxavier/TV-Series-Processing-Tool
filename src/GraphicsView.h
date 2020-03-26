#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>

#include "Face.h"

class GraphicsView: public QGraphicsView
{
  Q_OBJECT

 public:
  GraphicsView(QWidget *parent = 0);
  ~GraphicsView();

  public slots:
    void displayFaces(const QList<Face> &faces);
    
 protected:
  void paintEvent(QPaintEvent *event);

 private:
  QList<Face> m_faces;
};

#endif
