#include "histfacadepainter.h"
#include <histfacade.h>
#include <histpainter.h>

void HistFacadePainter::paint()
{
    _painter->setRect(_left, _bottom, _width, _height);
    _painter->setFreqRange(_min, _max);
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

void HistFacadePainter::setFreqRange(float min, float max)
{
    _min = min;
    _max = max;
}

void HistFacadePainter::setHist(std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    _histFacade = histFacade;
    _displayDims = displayDims;
    if (std::dynamic_pointer_cast<const HistNullFacade>(histFacade)) {
        _painter = std::make_shared<HistNullPainter>();
    } else if (2 == displayDims.size()) {
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
