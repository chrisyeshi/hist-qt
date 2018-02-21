#include "timelineview.h"
#include <QBoxLayout>
#include <QMouseEvent>
#include <QSlider>
#include <QScrollBar>
#include <painter.h>

namespace {

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return std::max(lo, std::min(hi, v));
}

} // unnamed namespace

const QColor TimelineView::_hoveredColor = QColor(231, 76, 60, 50);
const QColor TimelineView::_selectedColor = QColor(231, 76, 60, 150);

TimelineView::TimelineView(QWidget *parent)
  : OpenGLWidget(parent), _currStep(0), _hoveredStep(-1) {
    setFocusPolicy(Qt::ClickFocus);
    setMinimumHeight(100);
    setMouseTracking(true);
    delayForInit([this]() {
        glClearColor(1.f, 1.f, 1.f, 1.f);
    });
}

void TimelineView::setTimeStep(int timeStep) {
    _currStep = timeStep;
}

void TimelineView::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    static std::vector<QColor> dimToColor =
            {QColor(50, 69, 180, 100), QColor(39, 174, 96, 100)};
    // setup painter
    Painter painter(this);
    painter.setPen(QPen(Qt::black, 0.25f));
    painter.setFont(QFont("mono", 8.f));
    // variables
    float chartLineWidth = 0.25f;
    float plottingLineWidth = 4.f;
    float margin = 4.f;
    float legendSpacing = 2.f;
    QRectF leftLabelRect =
            painter.boundingRect(
                0, 0, 100, 100, Qt::AlignBottom | Qt::AlignLeft, tr("max"));
    float leftLabelWidth = leftLabelRect.width();
    float textHeight = leftLabelRect.height();
    QRect bottomLabelRect =
            painter.boundingRect(
                0, 0, 100, 100,
                Qt::AlignBottom | Qt::AlignLeft,
                QString::number(0.012345678, 'g', 5));
    float bottomLabelWidth = bottomLabelRect.width();
    float plotLeftWidth = 2 * margin + leftLabelWidth;
    float plotTopHeight = 2 * margin + textHeight;
    float plotBottomHeight = 2 * margin + textHeight;
    float plotRightWidth = margin;
    float plotLeft = plotLeftWidth;
    float plotTop = plotTopHeight;
    float plotBottom = height() - plotBottomHeight;
    float plotRight = width() - plotRightWidth;
    float plotWidth = plotRight - plotLeft;
    float plotHeight = plotBottom - plotTop;
    _plotRect = QRectF(plotLeft, plotTop, plotWidth, plotHeight);
    float stepWidth = plotWidth / _timeSteps.nSteps();
    int bottomLabelStep = int(std::ceil(bottomLabelWidth / stepWidth) + 0.5f);
    // averages
    for (unsigned int i = 0; i < _displayDims.size(); ++i) {
        unsigned iDim = _displayDims[i];
        float vMin = std::numeric_limits<float>::max();
        float vMax = std::numeric_limits<float>::lowest();
        for (int iStep = 0; iStep < _dataStats.size(); ++iStep) {
            const auto& volumeStats = _dataStats[iStep][_histConfig.name()];
            float average = volumeStats.means.at(_histConfig.vars[iDim]);
            vMin = std::min(vMin, average);
            vMax = std::max(vMax, average);
        }
        painter.setPen(QPen(dimToColor[i], plottingLineWidth));
        for (unsigned int iStep = 0; iStep < _dataStats.size(); ++iStep) {
            const auto& volumeStats = _dataStats[iStep][_histConfig.name()];
            float average = volumeStats.means.at(_histConfig.vars[iDim]);
            float ratio = (average - vMin) / (vMax - vMin);
            float top = plotTop + 0.5f * plottingLineWidth;
            float bottom = plotBottom - 0.5f * plottingLineWidth;
            float y = ratio * (top - bottom) + bottom;
            float xBeg =
                    plotLeft + iStep * stepWidth + 0.5f * plottingLineWidth;
            float xEnd = plotLeft + (iStep + 1) * stepWidth
                    - 0.5f * plottingLineWidth;
            painter.drawLine(QLineF(xBeg, y, xEnd, y));
        }
    }
    // plot box
    painter.setPen(QPen(Qt::black, chartLineWidth));
    painter.drawRect(plotLeft, plotTop, plotWidth, plotHeight);
    // tick marks
    painter.setPen(QPen(Qt::black, chartLineWidth));
    for (int iStep = 1; iStep < _timeSteps.nSteps(); ++iStep) {
        float x = plotLeft + iStep * stepWidth;
        painter.drawLine(QLineF(x, plotTop, x, plotBottom));
    }
    // horizontal labels
    for (int iStep = 0; iStep < _timeSteps.nSteps(); iStep += bottomLabelStep) {
        painter.drawText(
                plotLeft + iStep * stepWidth, plotBottom, stepWidth,
                plotBottomHeight, Qt::AlignCenter,
                QString::fromStdString(_timeSteps.asString(iStep)));
    }
    // vertical labels
    painter.drawText(
            0, plotTop, plotLeftWidth, textHeight, Qt::AlignCenter, tr("max"));
    painter.drawText(0, plotBottom - textHeight, plotLeftWidth, textHeight,
            Qt::AlignCenter, tr("min"));
    // legends
    float legendX = plotLeft;
    float legendBoxSide = plotTopHeight - 2 * margin;
    for (unsigned int i = 0; i < _displayDims.size(); ++i) {
        unsigned int iDim = _displayDims[i];
        painter.fillRect(
                legendX, margin, legendBoxSide, legendBoxSide, dimToColor[i]);
        legendX += legendBoxSide + legendSpacing;
        QRectF rect =
                painter.boundingRect(
                    QRect(legendX, 0, 100, plotTopHeight),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString::fromStdString(_histConfig.vars[iDim]));
        painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter,
                QString::fromStdString(_histConfig.vars[iDim]));
        legendX += rect.width() + 10 * legendSpacing;
    }
    // highlight steps
    painter.fillRect(
            QRectF(plotLeft + _currStep * stepWidth, 0, stepWidth, height()),
                _selectedColor);
    if (_hoveredStep > -1) {
        painter.fillRect(
                QRectF(plotLeft + _hoveredStep * stepWidth, 0, stepWidth,
                    height()),
                _hoveredColor);
    }
}

void TimelineView::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        setSelectedStep(localPosToStep(event->localPos()));
    } else {
        _hoveredStep = localPosToStep(event->localPos());
    }
    update();
}

void TimelineView::mousePressEvent(QMouseEvent *event) {
    _hoveredStep = localPosToStep(event->localPos());
    setSelectedStep(localPosToStep(event->localPos()));
    update();
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event) {

}

void TimelineView::leaveEvent(QEvent *) {
    _hoveredStep = -1;
    update();
}

void TimelineView::keyPressEvent(QKeyEvent *event) {
    if (Qt::Key_Right == event->key()) {
        setSelectedStep(clamp(_currStep + 1, 0, _timeSteps.nSteps() -1));
        update();
        return;
    }
    if (Qt::Key_Left == event->key()) {
        setSelectedStep(clamp(_currStep - 1, 0, _timeSteps.nSteps() -1));
        update();
        return;
    }
    OpenGLWidget::keyPressEvent(event);
}

int TimelineView::localPosToStep(QPointF pos) const {
    float stepWidth = _plotRect.width() / _timeSteps.nSteps();
    return clamp(
            int((pos.x() - _plotRect.x()) / stepWidth),
            0,
            _timeSteps.nSteps() - 1);
}

void TimelineView::setSelectedStep(int step) {
    _currStep = step;
    emit timeStepChanged(_currStep);
}

//void TimelineView::setStats(std::vector<HistFacadeVolume::Stats> stats) {
//    _timePlotView->setStats(stats);
//    _timePlotView->update();
//}
