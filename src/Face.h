#ifndef FACE_H
#define FACE_H

#include <QRect>
#include <QPoint>

class Face
{
 public:
  Face(const QRect &bbox, const QList<QPoint> &landmarks, int id, const QString &name);

  QRect getBbox() const;
  QList<QPoint> getLandmarks() const;
  int getId() const;
  QString getName() const;
  
  private:
  QRect m_bbox;
  QList<QPoint> m_landmarks;
  int m_id;
  QString m_name;
};

#endif
