#ifndef DIFFHISTOGRAPH_H
#define DIFFHISTOGRAPH_H

#include <QWidget>
#include <QList>
#include <QColor>
#include <opencv2/core/core.hpp>

class DiffHistoGraph: public QWidget
{
  Q_OBJECT

 public:
  DiffHistoGraph(QColor color, bool minusY = false, int scale = 4, int width = 380, int height = 100, QWidget *parent = 0);

  public slots:
  void appendDist(qreal dist);
  void setScale(int scale);

  protected:
  void paintEvent(QPaintEvent *event);

 private:
  int m_height;
  qreal m_scale;
  bool m_minusY;
  QList<qreal> m_distList;
  QColor m_color;
};

#endif
