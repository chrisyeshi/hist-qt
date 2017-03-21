#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <widget.h>

class QSlider;
class QScrollBar;

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
    QScrollBar* _timeSlider;
};

#endif // TIMELINEVIEW_H
