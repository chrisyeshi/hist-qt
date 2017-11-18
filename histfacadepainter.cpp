#include "histfacadepainter.h"
#include <histfacade.h>
#include <histpainter.h>

void HistFacadePainter::paint()
{
    _painter->setNormalizedViewport(_left, _bottom, _width, _height);
    setPainterNormalizedRect(_painter, normalizedRect());
    _painter->setFreqRange(_min, _max);
    _painter->setColorMap(
            _histFacade->selected() ?
                IHistPainter::YELLOW_BLUE :
                IHistPainter::GRAY_SCALE);
    _painter->paint();
}

void HistFacadePainter::setNormalizedViewport(
        float x, float y, float w, float h) {
    _left = x;
    _bottom = y;
    _width = w;
    _height = h;
}

void HistFacadePainter::setNormalizedRect(float x, float y, float w, float h) {
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

void HistFacadePainter::setRanges(std::vector<std::array<double, 2>> ranges) {
    _ranges = ranges;
}

std::array<float, 4> HistFacadePainter::normalizedRect() const {
    std::array<float, 4> rect = {_left, _bottom, _width, _height};
    if (_ranges.size() > 0) {
        auto dimRange = _histFacade->dimRange(_displayDims[0]);
        double lower =
                (dimRange[0] - _ranges[0][0]) / (_ranges[0][1] - _ranges[0][0]);
//                (_ranges[0][0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
        double upper =
                (dimRange[1] - _ranges[0][0]) / (_ranges[0][1] - _ranges[0][0]);
//                (_ranges[0][1] - dimRange[0]) / (dimRange[1] - dimRange[0]);
        rect[0] = _left + lower * _width;
        rect[2] = _width * (upper - lower);
    }
    if (_ranges.size() > 1) {
        auto dimRange = _histFacade->dimRange(_displayDims[1]);
        double lower =
                (dimRange[0] - _ranges[1][0]) / (_ranges[1][1] - _ranges[1][0]);
//                (_ranges[1][0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
        double upper =
                (dimRange[1] - _ranges[1][0]) / (_ranges[1][1] - _ranges[1][0]);
//                (_ranges[1][1] - dimRange[0]) / (dimRange[1] - dimRange[0]);
        rect[1] = _bottom + lower * _height;
        rect[3] = (upper - lower) * _height;
    }
    return rect;
}

void HistFacadePainter::setPainterNormalizedRect(
        std::shared_ptr<IHistPainter> painter, std::array<float, 4> rect) {
    painter->setNormalizedRect(rect[0], rect[1], rect[2], rect[3]);
}

//yy::vec4 HistFacadePainter::normlalizedViewport() const {
//    yy::vec4 viewport(0.f, 0.f, 1.f, 1.f);
//    if (_ranges.size() > 0) {
//        auto dimRange = _histFacade->dimRange(_displayDims[0]);
//        viewport[0] =
//                (_ranges[0][0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
//        viewport[2] =
//                (_ranges[0][1] - _ranges[0][0]) / (dimRange[1] - dimRange[0]);
//    }
//    if (_ranges.size() > 1) {
//        auto dimRange = _histFacade->dimRange(_displayDims[1]);
//        viewport[1] =
//                (_ranges[1][0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
//        viewport[3] =
//                (_ranges[1][1] - _ranges[1][0]) / (dimRange[1] - dimRange[0]);
//    }
//    return viewport;
//}
