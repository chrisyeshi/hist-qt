#ifndef HISTCHARTER_H
#define HISTCHARTER_H

#include <memory>
#include <array>
#include <vector>
#include <string>

class Hist;
class HistFacade;
class Hist2DTexturePainter;
class Hist1DVBOPainter;
class QPaintDevice;

/**
 * @brief The IHistCharter class
 */
class IHistCharter {
public:
    static std::shared_ptr<IHistCharter> create(
            std::shared_ptr<const HistFacade> histFacade = nullptr,
            std::vector<int> displayDims = {},
            QPaintDevice* paintDevice = nullptr);

public:
    virtual void chart() = 0;
    virtual void setSize(int width, int height, int devicePixelRatio) = 0;
    virtual void setRange(float vMin, float vMax) = 0;
    virtual std::string setMouseHover(float x, float y) = 0;

protected:
    virtual int devicePixelRatio() const = 0;
    virtual float devicePixelRatioF() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;

protected:
    float labelBottom() const { return 12.f * devicePixelRatioF(); }
    float labelLeft() const { return 12.f * devicePixelRatioF(); }
    float histLeft() const { return 40.f * devicePixelRatioF() + labelLeft(); }
    float histBottom() const {
        return 40.f * devicePixelRatioF() + labelBottom();
    }
    float histTop() const { return 5.f * devicePixelRatioF(); }
    float histRight() const { return 5.f * devicePixelRatioF(); }
    float histWidth() const { return width() - histLeft() - histRight(); }
    float histHeight() const { return height() - histBottom() - histTop(); }
};

class HistNullCharter : public IHistCharter {
public:
    virtual void chart() override {}
    virtual void setSize(int, int, int) override {}
    virtual void setRange(float, float) override {}
    virtual std::string setMouseHover(float, float) override { return ""; }

protected:
    virtual int devicePixelRatio() const override { return 1; }
    virtual float devicePixelRatioF() const override { return 1.f; }
    virtual int width() const override { return 1; }
    virtual int height() const override { return 1; }
};

/**
 * @brief The Hist2DFacadeCharter class
 */
class Hist2DFacadeCharter : public IHistCharter {
public:
    Hist2DFacadeCharter(std::shared_ptr<const HistFacade> histFacade,
            std::array<int, 2> displayDims, QPaintDevice* paintDevice);

public:
    virtual void chart() override;
    virtual void setSize(
            int width, int height, int devicePixelRatio) override;
    virtual void setRange(float vMin, float vMax) override;
    virtual std::string setMouseHover(float x, float y) override;

protected:
    virtual int devicePixelRatio() const override { return _devicePixelRatio; }
    virtual float devicePixelRatioF() const override {
        return _devicePixelRatio;
    }
    virtual int width() const override { return _width * _devicePixelRatio; }
    virtual int height() const override { return _height * _devicePixelRatio; }

private:
    std::shared_ptr<const Hist> hist() const;

private:
    int _width = 0, _height = 0, _devicePixelRatio = 1;
    std::shared_ptr<const HistFacade> _histFacade;
    std::array<int, 2> _displayDims;
    QPaintDevice* _paintDevice = nullptr;
    std::shared_ptr<Hist2DTexturePainter> _histPainter;
    std::array<int, 2> _hoveredBin = {{ -1, -1 }};
};

/**
 * @brief The Hist1DFacadeCharter class
 */
class Hist1DFacadeCharter : public IHistCharter {
public:
    Hist1DFacadeCharter(
            std::shared_ptr<const HistFacade> histFacade, int displayDim,
            QPaintDevice* paintDevice);

public:
    virtual void chart() override;
    virtual void setSize(int width, int height, int devicePixelRatio) override;
    virtual void setRange(float vMin, float vMax) override;
    virtual std::string setMouseHover(float x, float) override;

protected:
    virtual int devicePixelRatio() const override { return _devicePixelRatio; }
    virtual float devicePixelRatioF() const override {
        return _devicePixelRatio;
    }
    virtual int width() const override { return _width * _devicePixelRatio; }
    virtual int height() const override { return _height * _devicePixelRatio; }

private:
    int _width = 0, _height = 0, _devicePixelRatio = 1;
    float _vMin, _vMax;
    std::shared_ptr<const HistFacade> _histFacade;
    int _displayDim;
    QPaintDevice* _paintDevice = nullptr;
    std::shared_ptr<Hist1DVBOPainter> _histPainter;
    int _hoveredBin = -1;
};

#endif // HISTCHARTER_H
