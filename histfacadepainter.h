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
    virtual void setNormalizedViewport(
            float x, float y, float w, float h) override;
    virtual void setNormalizedBox(float x, float y, float w, float h) override;
    virtual void setFreqRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption) override {}

public:
    void setHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);
    void setRanges(std::vector<std::array<double, 2>> ranges);

private:
    std::array<float, 4> normalizedBox() const;
    void setPainterNormalizedBox(
            std::shared_ptr<IHistPainter> painter, std::array<float, 4> rect);

private:
    std::shared_ptr<const HistFacade> _histFacade;
    std::shared_ptr<IHistPainter> _painter;
    std::vector<int> _displayDims;
    float _left, _bottom, _width, _height;
    float _min, _max;
    std::vector<std::array<double, 2>> _ranges;
};

#endif // HISTFACADEPAINTER_H
