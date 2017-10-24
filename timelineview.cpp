#include "timelineview.h"
#include <QBoxLayout>
#include <QSlider>
#include <QScrollBar>
#include <timeplotview.h>

TimelineView::TimelineView(QWidget *parent)
  : Widget(parent, Qt::Dialog)
{
    QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
    vBoxLayout->setMargin(0);
    _timePlotView = new TimePlotView(this);
    connect(_timePlotView, &TimePlotView::timeStepChanged,
            this, &TimelineView::timeStepChanged);
    vBoxLayout->addWidget(_timePlotView);
}

void TimelineView::setDataPool(DataPool* dataPool)
{
    _timePlotView->setDataPool(dataPool);
    _timePlotView->update();
}
