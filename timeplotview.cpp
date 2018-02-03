#include "timeplotview.h"
#include <data/DataPool.h>
#include <painter.h>
#include <QMouseEvent>

namespace {

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return std::max(lo, std::min(hi, v));
}

} // namespace unnamed

const QColor TimePlotView::_hoveredColor = QColor(231, 76, 60, 50);
const QColor TimePlotView::_selectedColor = QColor(231, 76, 60, 150);

TimePlotView::TimePlotView(QWidget *parent)
  : OpenGLWidget(parent)
  , _data(nullptr)
  , _hoveredStep(-1)
  , _selectedStep(0) {
    setFocusPolicy(Qt::ClickFocus);
    setMinimumHeight(30);
    setMouseTracking(true);
    delayForInit([this]() {
        glClearColor(1.f, 1.f, 1.f, 1.f);
    });
}

void TimePlotView::setDataPool(DataPool *dataPool)
{
    _data = dataPool;
    _hoveredStep = -1;
    _selectedStep = 0;
    update();
}

void TimePlotView::setTimeStep(int timeStep) {
    _selectedStep = timeStep;
    update();
}

void TimePlotView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!_data)
        return;
//    int deviceRatio = devicePixelRatio();
//    float deviceRatioF = devicePixelRatioF();
    int deviceRatio = 1;
    float deviceRatioF = 1.f;
    int w = width() * deviceRatio;
    int h = height() * deviceRatio;
    Painter painter(this);
    painter.setPen(QPen(Qt::black, 0.25f));
    painter.setFont(QFont("mono", 8.f * deviceRatioF));
    // label rect
    QRect labelRect =
            painter.boundingRect(
                0, 0, 100, 100,
                Qt::AlignBottom | Qt::AlignLeft,
                QString::number(0.012345678, 'g', 5));
    // variables
    int nSteps = _data->numSteps();
    float stepWidth = float(w) / float(nSteps);
    float left = 0.f;
    float top = 0.f;
    float labelWidth = labelRect.width();
    float bottomHeight = labelRect.height() + 1.f;
    float plotBottom = float(h) - bottomHeight;
    int labelStep = int(std::ceil(labelWidth / stepWidth) + 0.5f);
    // lines
    for (int iStep = 1; iStep < nSteps; ++iStep) {
        painter.drawLine(
                QLineF(float(iStep) * stepWidth, top,
                    float(iStep) * stepWidth, plotBottom));
    }
    painter.drawLine(QLineF(left, plotBottom, float(w), plotBottom));
    // labels
    for (int iStep = 0; iStep < nSteps; iStep += labelStep) {
        painter.drawText(left + stepWidth * iStep, h - bottomHeight,
                labelWidth, bottomHeight, Qt::AlignTop | Qt::AlignLeft,
                QString::number(iStep, 'g', 5));
//                QString::number(_data->timeStepDouble(iStep), 'g', 5));
//                QString::fromStdString(_data->timeStepStr(iStep)));
//                QString::number(iStep * _data->interval(), 'g', 5));
    }
    // selected step
    painter.fillRect(QRectF(_selectedStep * stepWidth, top, stepWidth, h),
            _selectedColor);
    // hovered step
    if (_hoveredStep > -1) {
        painter.fillRect(QRectF(_hoveredStep * stepWidth, top, stepWidth, h),
                _hoveredColor);
    }
//    painter.paint();
}

void TimePlotView::mouseMoveEvent(QMouseEvent *event)
{
    if (!_data)
        return;
    if (event->buttons() & Qt::LeftButton) {
        setSelectedStep(
                localPosToStep(event->localPos().x(), event->localPos().y()));
    } else {
        _hoveredStep =
                localPosToStep(event->localPos().x(), event->localPos().y());
    }
    update();
}

void TimePlotView::mousePressEvent(QMouseEvent *event)
{
    if (!_data)
        return;
    _hoveredStep = -1;
    setSelectedStep(
            localPosToStep(event->localPos().x(), event->localPos().y()));
    update();
}

void TimePlotView::mouseReleaseEvent(QMouseEvent */*event*/) {

}

void TimePlotView::leaveEvent(QEvent *) {
    _hoveredStep = -1;
    update();
}

void TimePlotView::keyPressEvent(QKeyEvent *event) {
    if (!_data) {
        OpenGLWidget::keyPressEvent(event);
        return;
    }
    if (Qt::Key_Right == event->key()) {
        qInfo() << "TimePlotView key right pressed.";
        setSelectedStep(clamp(_selectedStep + 1, 0, _data->numSteps() -1));
        update();
        return;
    }
    if (Qt::Key_Left == event->key()) {
        qInfo() << "TimePlotView key left pressed.";
        setSelectedStep(clamp(_selectedStep - 1, 0, _data->numSteps() -1));
        update();
        return;
    }
    OpenGLWidget::keyPressEvent(event);
}

int TimePlotView::localPosToStep(float x, float /*y*/) const
{
    int w = width();
    int nSteps = _data->numSteps();
    float stepWidth = float(w) / float(nSteps);
    return clamp(int(x / stepWidth), 0, nSteps - 1);
}

void TimePlotView::setSelectedStep(int step)
{
    _selectedStep = step;
    emit timeStepChanged(_selectedStep);
}
