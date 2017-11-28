#include "histview.h"
#include <QToolTip>
#include <QMouseEvent>
#include <painter.h>
#include <histfacade.h>
#include <histpainter.h>
#include <histcharter.h>
#include <data/Histogram.h>

HistView::HistView(QWidget *parent)
  : OpenGLWidget(parent)
  , _histCharter(IHistCharter::create(nullptr, {})){
    setMouseTracking(true);
    setMinimumSize(200, 200);
    QSizePolicy policy(sizePolicy());
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    delayForInit([this]() {
        glClearColor(1.f, 1.f, 1.f, 1.f);
    });
    qRegisterMetaType<IHistCharter::HistRangesMap>("HistRangesMap");
}

void HistView::setHist(std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    delayForInit([this, histFacade, displayDims]() {
        _histCharter =
                IHistCharter::create(
                    histFacade, displayDims, this /*paintDevice*/,
                    [this](float x, float y, const std::string& label) {
            if (label.empty()) {
                QToolTip::hideText();
            } else {
                QPoint pos = this->mapToGlobal(QPoint(x, y));
                QToolTip::showText(pos, tr("placeholder"));
                QToolTip::showText(pos, QString::fromStdString(label));
            }
        });
        _histCharter->selectedHistRangesChanged =
                [this](IHistCharter::HistRangesMap histRangesMap) {
            emit selectedHistRangesChanged(histRangesMap);
        };
        if (!histFacade)
            return;
        auto hist2d = histFacade->hist(displayDims);
        float vMin = std::numeric_limits<float>::max();
        float vMax = std::numeric_limits<float>::lowest();
        for (auto iBin = 0; iBin < hist2d->nBins(); ++iBin) {
            auto v = hist2d->binPercent(iBin);
            vMin = std::min(vMin, float(v));
            vMax = std::max(vMax, float(v));
        }
        _histCharter->setRange(vMin, vMax);
    });
}

void HistView::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _histCharter->setSize(width(), height(), 1);
    _histCharter->chart();
}

void HistView::mousePressEvent(QMouseEvent *event) {
    _histCharter->mousePressEvent(event);
    update();
}

void HistView::mouseReleaseEvent(QMouseEvent *event) {
    _histCharter->mouseReleaseEvent(event);
    update();
}

void HistView::mouseMoveEvent(QMouseEvent *event) {
    _histCharter->mouseMoveEvent(event);
    update();
}

void HistView::leaveEvent(QEvent *event) {
    _histCharter->leaveEvent(event);
    update();
}
