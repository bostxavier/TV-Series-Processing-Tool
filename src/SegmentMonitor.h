#ifndef SEGMENTMONITOR_H
#define SEGMENTMONITOR_H

#include <QWidget>
#include <QMenu>

#include "Shot.h"
#include "SpeechSegment.h"
#include "VideoFrame.h"

class SegmentMonitor: public QWidget
{
  Q_OBJECT

 public:
  SegmentMonitor(QWidget *parent = 0);
  void reset();
  qreal getRatio() const;
  void setAnnotMode(bool annotMode);

  public slots:
    void processShots(QList<Segment *> shots, Segment::Source source);
    void processSpeechSegments(QList<Segment *> speechSegments, Segment::Source source);
    void processRefSpeakers(const QStringList &speakers);
    void positionChanged(qint64 position);
    void updateDuration(qint64 duration);
    void setWidth(int width);
    void showContextMenu(const QPoint &point);
    void addNewSpeaker();

 signals:
    void playSegments(QList<QPair<qint64, qint64>> utterances);

 protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

 private:
    QString processSpkLabel(QString spkLabel);
    void clearSpeakers();
    void computeSpeechRate(QList<Segment *> speechSegments, qint64 end);
    QList<qint64> getCharTimeStamps(const QStringList &words, qint64 start, qint64 end);
    QString reduceSpkLabel(const QString &spkLbl);


    Segment::Source m_source;
    QAction *m_sep;
    QMenu *m_speakers;
    QAction *m_newSpeakerAct;
    QAction *m_anonymizeAct;
    QAction *m_removeAct;
    QAction *m_insertAct;
    QAction *m_splitAct;
    QList<QList <Segment *> > m_segmentationList;
    QList<qreal> m_shotSize;
    QList<QPair<qint64, qreal> > m_speechRate;
    QList<QPair<qint64, qreal> > m_musicRate;
    int m_width;
    qreal m_ratio;
    int m_step;
    int m_segmentHeight;
    qint64 m_duration;
    qint64 m_currPosition;
    QList<QColor> m_colors;
    QHash<QString, QColor> m_segColors;
    int m_idxColor;
    QStringList m_selectedLabels;
    bool m_annotMode;
    qint64 m_prevPosition;
};

#endif
