#ifndef SEGMENT_H
#define SEGMENT_H

#include <QString>
#include <QList>
#include <QJsonObject>

#include "SpkInteractDialog.h"

class Segment
{
 public:
  enum Source {
    Manual, Automatic, Both
  };

  Segment(Segment *parent = 0);
  Segment(qint64 position = 0, Segment *parent = 0, Source source = Manual);
  virtual ~Segment();
  virtual void read(const QJsonObject &json);
  virtual void write(QJsonObject &json) const;

  Segment *child(int row);
  void appendChild(Segment *childSegment);
  void insertChild(int row, Segment *segment);
  bool insertChildren(int position, QList<Segment *> segments);
  void removeChild(int row);
  bool removeChildren(int position, int count);
  int childCount() const;
  virtual QString display() const = 0;
  int row() const;
  Segment *parent();
  Segment *getNextSegment();

  qint64 getPosition() const;
  QString getFormattedPosition() const;
  QList<Segment *> getChildren() const;
  int getHeight() const;
  Source getSource() const;
  void setPosition(qint64 position);
  void setSource(Source source);

  void setChildren(const QList<Segment *> &children);
  void setParent(Segment *parent);
  int childIndexFromPosition(qint64 position);
  void clearChildren();
  void splitChildren(QList<Segment *> &subList1, QList<Segment *> &subList2, int pos, Segment *newParent);

  virtual void setLabel(const QString &label, Segment::Source source);
  virtual void addInterLoc(const QString &speaker, SpkInteractDialog::InteractType type);
  virtual void resetInterLocs(SpkInteractDialog::InteractType type);
  virtual void setEnd(qint64 end);
  virtual int getNumber() const;
  virtual QString getLabel(Segment::Source source) const;
  virtual QStringList getInterLocs(SpkInteractDialog::InteractType type) const;
  virtual qint64 getEnd() const;
  virtual int getCamera(Segment::Source source) const;

 protected:
  qint64 m_position;
  Source m_source;
  QList<Segment *> m_childSegments;
  Segment *m_parentSegment;

 private:
  
};

#endif
