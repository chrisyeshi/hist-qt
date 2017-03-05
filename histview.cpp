#include "histview.h"

#include <histpainter.h>
#include <data/Histogram.h>

HistView::HistView(QWidget *parent)
  : OpenGLWidget(parent)
{

}

Hist2DView::Hist2DView(QWidget *parent)
  : HistView(parent)
{

}

void Hist2DView::setHist(std::shared_ptr<const Hist> hist)
{
    _hist = hist;
    delayForInit([this]() {
        auto hist2d = std::static_pointer_cast<const Hist2D>(_hist);
        float vMin = std::numeric_limits<float>::max();
        float vMax = std::numeric_limits<float>::lowest();
        for (auto v : hist2d->values()) {
            vMin = std::min(vMin, float(v));
            vMax = std::max(vMax, float(v));
        }
        _painter = std::make_shared<Hist2DPainter>(hist2d.get());
        _painter->initialize();
        _painter->setColorMap(HistPainter::YELLOW_BLUE);
        _painter->setRect(0.f, 0.f, 1.f, 1.f);
        _painter->setRange(vMin, vMax);
    });
}

void Hist2DView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (_painter)
        _painter->paint();
}

void Hist2DView::resizeGL(int w, int h)
{

}
