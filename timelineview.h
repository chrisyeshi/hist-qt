#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <widget.h>

class QSlider;
class QScrollBar;
class TimePlotView;
class DataPool;

class TimelineView : public Widget
{
    Q_OBJECT
public:
    explicit TimelineView(QWidget *parent = 0);

public:
    void setDataPool(DataPool* dataPool);

signals:
    void timeStepChanged(int);

private:
    TimePlotView* _timePlotView;
};

#endif // TIMELINEVIEW_H
