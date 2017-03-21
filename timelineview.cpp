#include "timelineview.h"
#include <QBoxLayout>
#include <QSlider>
#include <QScrollBar>

TimelineView::TimelineView(QWidget *parent)
  : Widget(parent, Qt::Dialog)
{
    QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
    _timeSlider = new QScrollBar(Qt::Horizontal, this);
    _timeSlider->setPageStep(1);
    vBoxLayout->addWidget(_timeSlider);
    connect(_timeSlider, &QScrollBar::valueChanged,
            this, &TimelineView::timeStepChanged);
}

void TimelineView::setNTimeSteps(int nTimeSteps)
{
    _timeSlider->blockSignals(true);
    _timeSlider->setRange(0, nTimeSteps - 1);
    _timeSlider->blockSignals(false);
}
