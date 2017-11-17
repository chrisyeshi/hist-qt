#ifndef HISTFACADEPAINTER_H
#define HISTFACADEPAINTER_H

#include <histpainter.h>
#include <memory>
#include <vec.h>

class HistFacade;

class HistFacadePainter : public IHistPainter {
public:
    HistFacadePainter() : _histFacade(nullptr) {}

public:
    virtual void initialize() override {}
    virtual void paint() override;
    virtual void setRect(float x, float y, float w, float h) override;
    virtual void setFreqRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption) override {}
    void setHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);

private:
    std::shared_ptr<const HistFacade> _histFacade;
    std::shared_ptr<IHistPainter> _painter;
    std::vector<int> _displayDims;
    float _left, _bottom, _width, _height;
    float _min, _max;
    std::vector<yy::vec2> _ranges;
};

#endif // HISTFACADEPAINTER_H
