#include "histfacadepainter.h"
#include <histfacade.h>
#include <histpainter.h>

void HistFacadePainter::paint()
{
    _painter->setRect(_left, _bottom, _width, _height);
    _painter->setRange(_min, _max);
    _painter->setColorMap(
            _histFacade->selected() ?
                IHistPainter::YELLOW_BLUE :
                IHistPainter::GRAY_SCALE);
    _painter->paint();
}

void HistFacadePainter::setRect(float x, float y, float w, float h)
{
    _left = x;
    _bottom = y;
    _width = w;
    _height = h;
}

void HistFacadePainter::setRange(float min, float max)
{
    _min = min;
    _max = max;
}

void HistFacadePainter::setHist(std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    assert(histFacade->nDim() >= int(displayDims.size()));
    _histFacade = histFacade;
    _displayDims = displayDims;
    if (2 == displayDims.size()) {
        auto painter = std::make_shared<Hist2DTexturePainter>();
        painter->initialize();
        painter->setTexture(_histFacade->texture(_displayDims));
        _painter = painter;
    } else if (1 == displayDims.size()) {
        auto painter = std::make_shared<Hist1DVBOPainter>();
        painter->initialize();
        painter->setVBO(_histFacade->vbo(_displayDims));
        _painter = painter;
    } else {
        assert(false);
    }
}
