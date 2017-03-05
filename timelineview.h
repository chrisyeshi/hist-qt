#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <widget.h>

class QSlider;

class TimelineView : public Widget
{
    Q_OBJECT
public:
    explicit TimelineView(QWidget *parent = 0);

public:
    void setNTimeSteps(int nTimeSteps);

signals:
    void timeStepChanged(int);

private:
    QSlider* _timeSlider;
};

#endif // TIMELINEVIEW_H
