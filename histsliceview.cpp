#include "histsliceview.h"
#include <data/histgrid.h>
#include <histfacadegrid.h>
#include <histpainter.h>
#include <histfacadepainter.h>
#include <painter.h>
#include <QMouseEvent>

/**
 * @brief HistSliceView::HistSliceView
 * @param parent
 */
HistSliceView::HistSliceView(QWidget *parent)
  : OpenGLWidget(parent)
  , _histRect(new HistFacadeRect())
{
    this->setMinimumSize(200, 200);
    this->setMouseTracking(true);
}

void HistSliceView::setHistRect(std::shared_ptr<HistFacadeRect> histRect)
{
    _histRect = histRect;
    /// TODO: right now draw the AB 2d histogram.
    /// TODO: cache the collapsed 1d and 2d histograms in Hist3D.
    /// TODO: draw the histogram rect.
}

void HistSliceView::setHistDimensions(std::vector<int> histDims)
{
    _histDims = histDims;
}

void HistSliceView::setSpacingColor(QColor color)
{
    _spacingColor = color;
    delayForInit([this]() {
        QColor c = quarterColor();
        glClearColor(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    });
}

void HistSliceView::setHoveredHist(std::array<int, 2> rectId, bool hovered)
{
    _hoveredHistRectId = rectId;
    _histHovered = hovered;
}

void HistSliceView::setMultiHists(std::vector<std::array<int, 2>> rectIds) {
    _multiHistRectIds = rectIds;
}

void HistSliceView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (auto& painter : _histPainters)
        painter->paint();
    Painter painter(this);
//    Painter painter(scaledWidth(), scaledHeight());
    // multi selected histograms
    if (!_multiHistRectIds.empty()) {
        painter.setPen(QPen(fullColor(), 6.f * _spacing));
        for (auto rectId : _multiHistRectIds) {
            painter.setPen(QPen(fullColor(), 6.f * _spacing));
            painter.drawRect(
                    _border + rectId[0]
                        * perHistWidthPixels(scaledWidth())
                        + (rectId[0] + 0.5f) * _spacing,
                    _border + (_histRect->nHistY() - 1 - rectId[1])
                        * perHistHeightPixels(scaledHeight())
                        + (_histRect->nHistY() - 1 - rectId[1] - 1.f)
                        * _spacing,
                    perHistWidthPixels(scaledWidth()) + _spacing,
                    perHistHeightPixels(scaledHeight()) + _spacing);
        }
    }
    // hovered histogram
    if (_histHovered) {
        painter.setPen(QPen(quarterColor(), 6.f * _spacing));
        painter.drawRect(
                _border + _hoveredHistRectId[0]
                    * perHistWidthPixels(scaledWidth())
                    + (_hoveredHistRectId[0] + 0.5f) * _spacing,
                _border + (_histRect->nHistY() - 1 - _hoveredHistRectId[1])
                    * perHistHeightPixels(scaledHeight())
                    + (_histRect->nHistY() - 1 - _hoveredHistRectId[1] - 1.f)
                    * _spacing,
                perHistWidthPixels(scaledWidth()) + _spacing,
                perHistHeightPixels(scaledHeight()) + _spacing);
    }
//    painter.paint();
}

void HistSliceView::resizeGL(int /*w*/, int /*h*/)
{
    updateHistRects(scaledWidth(), scaledHeight());
}

void HistSliceView::mousePressEvent(QMouseEvent *event)
{
    _mousePress = event->localPos();
    _histRectIdPress =
            localPositionToHistRectId(
                event->localPos().x(), event->localPos().y());
}

void HistSliceView::mouseReleaseEvent(QMouseEvent *event)
{
    auto rectId =
            localPositionToHistRectId(
                event->localPos().x(), event->localPos().y());
    if (rectId != _histRectIdPress) {
        return;
    }
    if (Qt::ControlModifier & event->modifiers()) {
        emit histMultiSelect(rectId, _histDims);
        return;
    }
    emit histClicked(rectId, _histDims);
}

void HistSliceView::mouseMoveEvent(QMouseEvent *event)
{
    auto rectId =
            localPositionToHistRectId(
                event->localPos().x(), event->localPos().y());
    if (_hoveredHistRectId != rectId) {
        emit histHovered(rectId, _histDims, _histHovered);
    }
}

void HistSliceView::enterEvent(QEvent */*event*/)
{
    _histHovered = true;
    QOpenGLWidget::update();
}

void HistSliceView::leaveEvent(QEvent */*event*/)
{
    _histHovered = false;
    emit histHovered(_hoveredHistRectId, _histDims, _histHovered);
    QOpenGLWidget::update();
}

void HistSliceView::updateHistPainters()
{
    delayForInit([this]() {
        makeCurrent();
        _histPainters.resize(_histRect->nHist());
        for (auto x = 0; x < _histRect->nHistX(); ++x)
        for (auto y = 0; y < _histRect->nHistY(); ++y) {
            auto hist = _histRect->hist(x, y);
            // hist facade painter
            auto painter = std::make_shared<HistFacadePainter>();
            painter->initialize();
            painter->setHist(hist, _histDims);
            // range
            float vMin = std::numeric_limits<float>::max();
            float vMax = std::numeric_limits<float>::lowest();
            auto collapsedHist = hist->hist(_histDims);
            for (auto v : collapsedHist->values()) {
                vMin = std::min(vMin, float(v));
                vMax = std::max(vMax, float(v));
            }
            painter->setFreqRange(vMin, vMax);
            // put into array
            auto iPainter = x + _histRect->nHistX() * y;
            _histPainters[iPainter] = painter;
        }
        updateHistRects(scaledWidth(), scaledHeight());
        doneCurrent();
    });
}

float HistSliceView::perHistWidthPixels(int w)
{
    int nHistX = _histRect->nHistX();
    return (w - 2.f * _border - (nHistX - 1) * _spacing) / nHistX;
}

float HistSliceView::perHistHeightPixels(int h)
{
    int nHistY = _histRect->nHistY();
    return (h - 2.f * _border - (nHistY - 1) * _spacing) / nHistY;
}

QColor HistSliceView::fullColor() const {
    QColor color = _spacingColor;
    color.setAlphaF(0.75f * color.alphaF());
    return color;
}

QColor HistSliceView::quarterColor() const {
    QColor color = _spacingColor;
    color.setAlphaF(0.25f * color.alphaF());
    return color;
}

std::array<int,2> HistSliceView::localPositionToHistRectId(int x, int y) const {
    float nx = x / float(width());
    float ny = 1.f - y / float(height());
    float dx = 1.f / float(_histRect->nHistX());
    float dy = 1.f / float(_histRect->nHistY());
    int iHistX = std::max(0, std::min(_histRect->nHistX() - 1, int(nx / dx)));
    int iHistY = std::max(0, std::min(_histRect->nHistY() - 1, int(ny / dy)));
    return {{ iHistX, iHistY }};
}

void HistSliceView::updateHistRects(int w, int h)
{
    makeCurrent();
    int nHistX = _histRect->nHistX();
    int nHistY = _histRect->nHistY();
    for (auto x = 0; x < nHistX; ++x)
    for (auto y = 0; y < nHistY; ++y) {
        auto iPainter = x + nHistX * y;
        auto& painter = _histPainters[iPainter];
        float perHistWidthPixels = this->perHistWidthPixels(w);
        float perHistWidth = perHistWidthPixels / w;
        float perHistHeightPixels = this->perHistHeightPixels(h);
        float perHistHeight = perHistHeightPixels / h;
        float left = (_border + x * perHistWidthPixels + x * _spacing) / w;
        float bottom = (_border + y * perHistHeightPixels + y * _spacing) / h;
        painter->setNormalizedViewportAndRect(left, bottom, perHistWidth, perHistHeight);
    }
    doneCurrent();
}
