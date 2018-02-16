#ifndef HISTCHARTER_H
#define HISTCHARTER_H

#include <memory>
#include <array>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <QtMath>
#include <QFont>
#include <QFontMetricsF>
#include <QDebug>

class Hist;
class HistFacade;
class Hist2DTexturePainter;
class Hist1DVBOPainter;
class HistFacadePainter;
class QPaintDevice;
class QMouseEvent;
class QEvent;

/**
 * @brief The IHistCharter class
 */
class IHistCharter {
public:
    static std::shared_ptr<IHistCharter> create(
            std::shared_ptr<const HistFacade> histFacade = nullptr,
            std::vector<int> displayDims = {},
            std::function<void(float, float, const std::string&)> showLabel =
                [](float, float, const std::string&) {});

public:
    typedef std::map<int, std::array<float, 2>> HistRangesMap;
    std::function<void(HistRangesMap)> selectedHistRangesChanged;

public:
    virtual void chart(QPaintDevice* paintDevice) = 0;
    virtual void setSize(int width, int height, int devicePixelRatio) = 0;
    virtual void setNormalizedViewport(
            float left, float top, float width, float height) = 0;
    virtual void setFreqRange(float vMin, float vMax) = 0;
    virtual void setRanges(std::vector<std::array<double, 2>> varRanges) = 0;
    virtual void mousePressEvent(QMouseEvent*) = 0;
    virtual void mouseReleaseEvent(QMouseEvent*) = 0;
    virtual void mouseMoveEvent(QMouseEvent*) = 0;
    virtual void leaveEvent(QEvent*) = 0;

protected:
    class RectF {
    public:
        RectF(float left, float top, float width, float height)
              : _left(left), _top(top), _width(width), _height(height) {}
        void setRect(float left, float top, float width, float height) {
            _left = left;
            _top = top;
            _width = width;
            _height = height;
        }
        float left() const { return _left; }
        float top() const { return _top; }
        float width() const { return _width; }
        float height() const { return _height; }
        float right() const { return 1.f - left() - width(); }
        float bottom() const { return 1.f - top() - height(); }

    private:
        float _left, _top, _width, _height;
    };

protected:
    virtual int devicePixelRatio() const = 0;
    virtual float devicePixelRatioF() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual RectF viewport() const = 0;

protected:
    virtual float tickAngleDegree() const { return -45; }
    virtual float tickAngleRadian() const {
        return qDegreesToRadians(tickAngleDegree());
    }
    virtual QFont tickFont() const { return QFont("mono", 8.f); }
    virtual QFontMetricsF tickFontMetrics() const {
        return QFontMetricsF(tickFont());
    }
    virtual int maxTickCharCount() const { return 11; }
    virtual float tickWidth() const {
        return maxTickCharCount() * tickFontMetrics().averageCharWidth();
    }
    virtual float tickHeight() const { return tickFontMetrics().height(); }
    virtual float labelBottom() const {
        return (12.f + viewport().bottom() * height()) * devicePixelRatioF();
    }
    virtual float labelLeft() const {
        return (16.f + viewport().left() * width()) * devicePixelRatioF();
    }
    virtual float histLeft() const {
        auto tanAngle = std::abs(std::tan(tickAngleRadian()));
        auto extendedWidth = tickHeight() / tanAngle + tickWidth();
        return extendedWidth * tanAngle + labelLeft();
//        return 44.f * devicePixelRatioF() + labelLeft();
    }
    virtual float histBottom() const {
        auto tanAngle = std::abs(std::tan(tickAngleRadian()));
        auto extendedWidth = tickHeight() / tanAngle + tickWidth();
        return extendedWidth * tanAngle + labelBottom();
//        return 48.f * devicePixelRatioF() + labelBottom();
    }
    virtual float histTop() const {
        return (10.f + viewport().top() * height()) * devicePixelRatioF();
    }
    virtual float histRight() const {
        return (10.f + viewport().right() * width()) * devicePixelRatioF();
    }
    virtual float histWidth() const {
        return width() - histLeft() - histRight();
    }
    virtual float histHeight() const {
        return height() - histBottom() - histTop();
    }
    virtual double chartLeft() const {
        return viewport().left() * width() * devicePixelRatioF();
    }
    virtual double chartTop() const {
        return viewport().top() * height() * devicePixelRatioF();
    }
    virtual double chartWidth() const {
        return viewport().width() * width() * devicePixelRatioF();
    }
    virtual double chartHeight() const {
        return viewport().height() * height() * devicePixelRatioF();
    }
    virtual double chartBottom() const {
        return viewport().bottom() * height() * devicePixelRatioF();
    }
};

class HistNullCharter : public IHistCharter {
public:
    virtual void chart(QPaintDevice*) override {}
    virtual void setSize(int, int, int) override {}
    virtual void setNormalizedViewport(float, float, float, float) override {}
    virtual void setFreqRange(float, float) override {}
    virtual void setRanges(std::vector<std::array<double, 2>>) override {}
    virtual void mousePressEvent(QMouseEvent*) override {}
    virtual void mouseReleaseEvent(QMouseEvent*) override {}
    virtual void mouseMoveEvent(QMouseEvent*) override {}
    virtual void leaveEvent(QEvent*) override {}

protected:
    virtual int devicePixelRatio() const override { return 1; }
    virtual float devicePixelRatioF() const override { return 1.f; }
    virtual int width() const override { return 1; }
    virtual int height() const override { return 1; }
    virtual RectF viewport() const override {
        return RectF(0.f, 0.f, 1.f, 1.f);
    }
};

/**
 * @brief The Hist2DFacadeCharter class
 */
class Hist2DFacadeCharter : public IHistCharter {
public:
    Hist2DFacadeCharter(std::shared_ptr<const HistFacade> histFacade,
            std::array<int, 2> displayDims,
            std::function<void(float, float, const std::string&)> showLabel);

public:
    virtual void chart(QPaintDevice* paintDevice) override;
    virtual void setSize(
            int width, int height, int devicePixelRatio) override;
    virtual void setNormalizedViewport(
            float left, float top, float width, float height) override;
    virtual void setFreqRange(float vMin, float vMax) override;
    virtual void setRanges(
            std::vector<std::array<double, 2> > varRanges) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void leaveEvent(QEvent *event) override;

protected:
    virtual int devicePixelRatio() const override { return _devicePixelRatio; }
    virtual float devicePixelRatioF() const override {
        return _devicePixelRatio;
    }
    virtual int width() const override { return _width * _devicePixelRatio; }
    virtual int height() const override { return _height * _devicePixelRatio; }
    virtual RectF viewport() const override { return _viewport; }

protected:
    float labelBottom() const override {
        return _drawColormap
                ? 50.f + viewport().bottom() * height()
                : IHistCharter::labelBottom();
    }
    float colormapLeft() const {
        return 10.f + viewport().left() * width();
    }
    float colormapRight() const { return 10.f + viewport().right() * width(); }
    float colormapBottom() const {
        return 16.f + viewport().bottom() * height();
    }
    float colormapTop() const { return height() - labelBottom() + 12.f; }
    float colormapHeight() const {
        return height() - colormapTop() - colormapBottom();
    }
    float colormapWidth() const {
        return width() - colormapLeft() - colormapRight();
    }
    float colormapBottomPadding() const { return 2.f; }

private:
    std::shared_ptr<const Hist> hist() const;
    std::array<int, 2> posToBinIds(float x, float y) const;

private:
    int _width = 0, _height = 0, _devicePixelRatio = 1;
    std::shared_ptr<const HistFacade> _histFacade;
    std::array<int, 2> _displayDims;
    std::shared_ptr<Hist2DTexturePainter> _histPainter;
    std::function<void(float, float, const std::string&)> _showLabel;
    std::array<int, 2> _hoveredBin = {{ -1, -1 }};
    std::array<float, 2> _freqRange = {{ 0.f, 1.f }};
    bool _isMousePressed = false;
    bool _isBinSelected = false;
    std::array<int, 2> _selectedBinBeg = {{-1, -1}};
    std::array<int, 2> _selectedBinEnd = {{-1, -1}};
    RectF _viewport = RectF(0.f, 0.f, 1.f, 1.f);
    bool _drawColormap = true;
};

/**
 * @brief The Hist1DFacadeCharter class
 */
class Hist1DFacadeCharter : public IHistCharter {
public:
    Hist1DFacadeCharter(
            std::shared_ptr<const HistFacade> histFacade, int displayDim,
            std::function<void(float, float, const std::string&)> showLabel);

public:
    virtual void chart(QPaintDevice* paintDevice) override;
    virtual void setSize(int width, int height, int devicePixelRatio) override;
    virtual void setNormalizedViewport(
            float left, float top, float width, float height) override;
    virtual void setFreqRange(float vMin, float vMax) override;
    virtual void setRanges(
            std::vector<std::array<double, 2> > varRanges) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void leaveEvent(QEvent*) override;

protected:
    virtual int devicePixelRatio() const override { return _devicePixelRatio; }
    virtual float devicePixelRatioF() const override {
        return _devicePixelRatio;
    }
    virtual int width() const override { return _width * _devicePixelRatio; }
    virtual int height() const override { return _height * _devicePixelRatio; }
    virtual RectF viewport() const override { return _viewport; }

private:
    int posToBinId(float x) const;
    void drawHist(double left, double bottom, double width, double height);
    std::array<double, 4> normalizedBox() const;

private:
    int _width = 0, _height = 0, _devicePixelRatio = 1;
    float _vMin, _vMax;
    std::shared_ptr<const HistFacade> _histFacade;
    int _displayDim;
    QPaintDevice* _paintDevice = nullptr;
    std::shared_ptr<Hist1DVBOPainter> _histPainter;
    std::function<void(float, float, const std::string&)> _showLabel;
    int _hoveredBin = -1;
    bool _isMousePressed = false;
    bool _isBinSelected = false;
    int _selectedBinBeg = -1;
    int _selectedBinEnd = -1;
    RectF _viewport = RectF(0.f, 0.f, 1.f, 1.f);
    std::vector<std::array<double, 2>> _varRanges;
};

/**
 * @brief The HistFacadeCharter class
 */
class HistFacadeCharter {
public:
    HistFacadeCharter();

public:
    void chart(QPaintDevice* paintDevice);
    void paint(QPaintDevice* paintDevice = nullptr) { chart(paintDevice); }
    void setSize(float width, float height);
    void setNormalizedViewport(
            float left, float bottom, float width, float height);
    void setFreqRange(float vMin, float vMax);
    void setRanges(std::vector<std::array<double, 2>> ranges);
    void setHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);

private:
    std::shared_ptr<IHistCharter> _charter;
    float _width, _height;
    std::array<float, 4> _viewport;
    std::array<float, 2> _freqRange;
    std::vector<std::array<double, 2>> _varRanges;
};

#endif // HISTCHARTER_H
