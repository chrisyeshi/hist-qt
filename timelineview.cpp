#include "timelineview.h"
#include <QBoxLayout>
#include <QSlider>

TimelineView::TimelineView(QWidget *parent)
  : Widget(parent, Qt::Dialog)
{
    QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
    _timeSlider = new QSlider(Qt::Horizontal, this);
    vBoxLayout->addWidget(_timeSlider);
    connect(_timeSlider, &QSlider::valueChanged,
            this, &TimelineView::timeStepChanged);
}

void TimelineView::setNTimeSteps(int nTimeSteps)
{
    _timeSlider->setRange(0, nTimeSteps - 1);
    _timeSlider->setValue(0);
}
