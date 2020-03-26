#ifndef SOCIALNETWIDGET_H
#define SOCIALNETWIDGET_H

#include <QGLWidget>
#include <QMouseEvent>
#include <QVector3D>
#include <QMatrix4x4>
#include <QPainter>

#include "Vertex.h"
#include "Edge.h"

class SocialNetWidget: public QWidget
{
  Q_OBJECT

    public:
  SocialNetWidget(QWidget *parent = 0);
  void setVerticesViews(const QVector<QList<Vertex> > &verticesViews);
  void setEdgesViews(const QVector<QList<Edge> > &edgesViews);
  void setSceneRefs(const QVector<QPair<int, int> > &sceneRefs);
  void setLabelDisplay(bool checked);
  void selectVertices();
  void selectEdges();
  void setFilterWeight(qreal filterWeight);
  void setActiveVertices(int vId);
  void setTwoDim(bool twoDim);
  void updateNetView(int iScene);
  void updateWidget();

  public slots:
    void paintNextScene();
    void setSceneIndex(qint64 sceneIndex);

 signals:
    void resetPlayer();
    void positionChanged(qint64 position);

 protected:
    // void initializeGL();
    // void resizeGL(int width, int height);
    // void paintGL();
  void paintEvent(QPaintEvent *event);

  QSize minimumSizeHint() const;
  QSize sizeHint() const;
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);

  private:
  // void drawAxes();
  // void drawVertices();
  // void drawEdges();
  // void displayLabels();
  // void adjustViewPort(int width, int height);
  GLfloat * getRGBColor(const QColor &refColor, qreal weight);
  QVector3D getArcballVector(int x, int y);
  QPointF getObjectCoord(int x, int y);
  QPointF getWindowCoord(qreal x, qreal y);
  qreal getAngle(const QVector3D &v1, const QVector3D &v2);
  int normalizeX(qreal x);
  int normalizeY(qreal y);
  bool compareVertices(const Vertex &v1, const Vertex &v2);

  bool m_twoDim;

  QMatrix4x4 m_projMat;
  QMatrix4x4 m_mvMat;

  qreal m_scale;
  qreal m_edgeScale;
  QColor m_refColor;
  qreal m_filterWeight;

  QPoint m_lastPos;

  QVector<QList<Vertex> > m_verticesViews;
  QList<int> m_activeVertices;
  QList<int> m_selVertices;
  QVector<QList<GLfloat *> > m_verticesColor;

  QVector<QList<Edge> > m_edgesViews;
  QList<int> m_selEdges;
  QVector<QList<GLfloat *> > m_edgesColor;

  QVector<QPair<int, int> > m_sceneRefs;

  QList<QColor> m_colors;
  
  int m_sceneIndex;

  qreal m_minRadius;
  qreal m_maxRadius;
  bool m_displayLabels;
};

#endif
