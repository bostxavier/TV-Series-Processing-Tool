#ifndef NARRCHARTWIDGET_H
#define NARRCHARTWIDGET_H

#include <QWidget>
#include <QMap>
#include <QList>
#include <QPoint>
#include <QMouseEvent>

class NarrChartWidget: public QWidget
{
  Q_OBJECT

public:
  NarrChartWidget(QWidget *parent = 0);

  void setNarrChart(const QMap<QString, QVector<QPoint> > &narrChart);
  void setSnapshots(const QVector<QMap<QString, QMap<QString, qreal> > > &snapshots);
  void setSceneRefs(const QVector<QPair<int, int> > &sceneRefs);

  QPair<int, int> getSceneBound() const;
  int getXCoord(int iScene) const;

  public slots:
    void displayLines(bool dispLines);
    void displayArcs(bool dispArcs);

    signals:

  protected:
    void paintEvent(QPaintEvent *event);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

 private:
    QString storyLineSelected(QMouseEvent *event);
    QColor getRGBColor(const QColor &refColor, qreal weight, int grayLevel = 0);

    QVector<QMap<QString, QMap<QString, qreal> > > m_snapshots;
    QMap<QString, QVector<QPoint> > m_narrChart;
    QVector<QPair<int, int> > m_sceneRefs;

    QMap<QString, QColor> m_storyLineColors;

    int m_refX;
    int m_refY;
    int m_firstScene;
    int m_lastScene;
    int m_minHeight;
    int m_maxHeight;
    int m_sceneWidth;
    int m_vertSpace;
    QPoint m_pos;
    bool m_posFixed;
    QString m_selStoryLine;
    bool m_dispLines;
    bool m_dispArcs;
};

#endif
