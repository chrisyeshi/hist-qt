#include "histsliceview.h"
#include <data/histgrid.h>
#include <histfacadegrid.h>
#include <histpainter.h>
#include <QMouseEvent>

/**
 * @brief HistSliceView::HistSliceView
 * @param parent
 */
HistSliceView::HistSliceView(QWidget *parent)
  : OpenGLWidget(parent)
  , _histRect(new HistFacadeRect())
{
}

void HistSliceView::setHistRect(std::shared_ptr<HistFacadeRect> histRect)
{
    _histRect = histRect;
    /// TODO: right now draw the AB 2d histogram.
    /// TODO: cache the collapsed 1d and 2d histograms in Hist3D.
    /// TODO: draw the histogram rect.
}

//void HistSliceView::setSelectedHistMask(BoolMask2D selectedHistMask)
//{
//    _selectedHistMask = selectedHistMask;
//}

void HistSliceView::setHistDimensions(std::vector<int> histDims)
{
    _histDims = histDims;
}

void HistSliceView::update()
{
    delayForInit([this]() {
        updateHistPainters();
    });
    QOpenGLWidget::update();
}

void HistSliceView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int iHist = 0; iHist < int(_histPainters.size()); ++iHist) {
        auto& painter = _histPainters[iHist];
//        if (_selectedHistMask.isSelected(iHist)) {
        if (_histRect->hist(iHist)->selected()) {
            painter->setColorMap(HistPainter::YELLOW_BLUE);
        } else {
            painter->setColorMap(HistPainter::GRAY_SCALE);
        }
        painter->paint();
    }
}

void HistSliceView::resizeGL(int /*w*/, int /*h*/)
{

}

void HistSliceView::mouseReleaseEvent(QMouseEvent *event)
{
    float nx = event->localPos().x() / float(width());
    float ny = 1.f - event->localPos().y() / float(height());
    float dx = 1.f / float(_histRect->nHistX());
    float dy = 1.f / float(_histRect->nHistY());
    int iHistX = nx / dx;
    int iHistY = ny / dy;
    emit histClicked({iHistX, iHistY}, _histDims);
}

void HistSliceView::updateHistPainters()
{
    makeCurrent();
    _histPainters.resize(_histRect->nHist());
    for (auto x = 0; x < _histRect->nHistX(); ++x)
    for (auto y = 0; y < _histRect->nHistY(); ++y) {
        auto hist = _histRect->hist(x, y);
        /// TODO: use the texture directly from the HistFacade class.
        auto collapsedHist = hist->hist(_histDims);
//        auto collapsedHist = HistCollapser(hist).collapseTo(_histDims);
        /// TODO: handle histograms with different dimensions.
        auto hist2d = std::dynamic_pointer_cast<const Hist2D>(collapsedHist);
        assert(hist2d);
        auto iPainter = x + _histRect->nHistX() * y;
        auto& painter = _histPainters[iPainter];
        painter = std::make_shared<Hist2DPainter>(hist2d.get());
        painter->initialize();
        // rect
        float width = 1.f / float(_histRect->nHistX());
        float height = 1.f / float(_histRect->nHistY());
        float left = float(x) * width;
        float bottom = float(y) * height;
        painter->setRect(left, bottom, width, height);
        // range
        float vMin = std::numeric_limits<float>::max();
        float vMax = std::numeric_limits<float>::lowest();
        for (auto v : hist2d->values()) {
            vMin = std::min(vMin, float(v));
            vMax = std::max(vMax, float(v));
        }
        painter->setRange(vMin, vMax);
        // colormap
        // painter->setColormap(colormap);
    }
    doneCurrent();
}
