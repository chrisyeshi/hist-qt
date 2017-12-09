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
            QPaintDevice* paintDevice = nullptr,
            std::function<void(float, float, const std::string&)> showLabel =
                [](float, float, const std::string&) {});

public:
    typedef std::map<int, std::array<float, 2>> HistRangesMap;
    std::function<void(HistRangesMap)> selectedHistRangesChanged;

public:
    virtual void chart() = 0;
    virtual void setSize(int width, int height, int devicePixelRatio) = 0;
    virtual void setRange(float vMin, float vMax) = 0;
    virtual void mousePressEvent(QMouseEvent*) = 0;
    virtual void mouseReleaseEvent(QMouseEvent*) = 0;
    virtual void mouseMoveEvent(QMouseEvent*) = 0;
    virtual void leaveEvent(QEvent*) = 0;

protected:
    virtual int devicePixelRatio() const = 0;
    virtual float devicePixelRatioF() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;

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
    virtual float labelBottom() const { return 12.f * devicePixelRatioF(); }
    virtual float labelLeft() const { return 16.f * devicePixelRatioF(); }
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
    virtual float histTop() const { return 10.f * devicePixelRatioF(); }
    virtual float histRight() const { return 10.f * devicePixelRatioF(); }
    virtual float histWidth() const {
        return width() - histLeft() - histRight();
    }
    virtual float histHeight() const {
        return height() - histBottom() - histTop();
    }
};

class HistNullCharter : public IHistCharter {
public:
    virtual void chart() override {}
    virtual void setSize(int, int, int) override {}
    virtual void setRange(float, float) override {}
    virtual void mousePressEvent(QMouseEvent*) override {}
    virtual void mouseReleaseEvent(QMouseEvent*) override {}
    virtual void mouseMoveEvent(QMouseEvent*) override {}
    virtual void leaveEvent(QEvent*) override {}

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
            std::array<int, 2> displayDims, QPaintDevice* paintDevice,
            std::function<void(float, float, const std::string&)> showLabel);

public:
    virtual void chart() override;
    virtual void setSize(
            int width, int height, int devicePixelRatio) override;
    virtual void setRange(float vMin, float vMax) override;
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

protected:
    float labelBottom() const override { return 50.f; }
    float colormapLeft() const { return 10.f; }
    float colormapRight() const { return 10.f; }
    float colormapBottom() const { return 16.f; }
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

private:
    int _width = 0, _height = 0, _devicePixelRatio = 1;
    std::shared_ptr<const HistFacade> _histFacade;
    std::array<int, 2> _displayDims;
    QPaintDevice* _paintDevice = nullptr;
    std::shared_ptr<Hist2DTexturePainter> _histPainter;
    std::function<void(float, float, const std::string&)> _showLabel;
    std::array<int, 2> _hoveredBin = {{ -1, -1 }};
    std::array<float, 2> _freqRange = {{ 0.f, 1.f }};
};

/**
 * @brief The Hist1DFacadeCharter class
 */
class Hist1DFacadeCharter : public IHistCharter {
public:
    Hist1DFacadeCharter(
            std::shared_ptr<const HistFacade> histFacade, int displayDim,
            QPaintDevice* paintDevice,
            std::function<void(float, float, const std::string&)> showLabel);

public:
    virtual void chart() override;
    virtual void setSize(int width, int height, int devicePixelRatio) override;
    virtual void setRange(float vMin, float vMax) override;
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

private:
    int posToBinId(float x) const;

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
};

#endif // HISTCHARTER_H
