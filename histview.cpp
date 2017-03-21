#include "histview.h"
#include <QToolTip>
#include <QMouseEvent>
#include <painter.h>
#include <histfacade.h>
#include <histpainter.h>
#include <histcharter.h>
#include <data/Histogram.h>

HistView::HistView(QWidget *parent)
  : OpenGLWidget(parent) {
    setMouseTracking(true);
    delayForInit([this]() {
        glClearColor(1.f, 1.f, 1.f, 1.f);
    });
}

void HistView::setHist(std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    delayForInit([this, histFacade, displayDims]() {
        _histCharter = IHistCharter::create(histFacade, displayDims);
        auto hist2d = histFacade->hist(displayDims);
        float vMin = std::numeric_limits<float>::max();
        float vMax = std::numeric_limits<float>::lowest();
        for (auto v : hist2d->values()) {
            vMin = std::min(vMin, float(v));
            vMax = std::max(vMax, float(v));
        }
        _histCharter->setRange(vMin, vMax);
    });
}

void HistView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _histCharter->setSize(width(), height(), devicePixelRatio());
    _histCharter->chart();
}

void HistView::mouseMoveEvent(QMouseEvent *event)
{
    std::string label =
            _histCharter->setMouseHover(
                event->localPos().x(), event->localPos().y());
    if (label.empty())
        QToolTip::hideText();
    else {
        QToolTip::showText(event->globalPos(), "label");
        QToolTip::showText(event->globalPos(), QString::fromStdString(label));
    }
    update();
}

void HistView::leaveEvent(QEvent *)
{
    _histCharter->setMouseHover(-1.f, -1.f);
    update();
}
