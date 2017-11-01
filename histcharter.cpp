#include "histcharter.h"
#include <cassert>
#include <histfacade.h>
#include <histpainter.h>
#include <painter.h>

/**
 * @brief IHistCharter::create
 * @param histFacade
 * @param displayDims
 * @return
 */
std::shared_ptr<IHistCharter> IHistCharter::create(
        std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims, QPaintDevice *paintDevice) {
    if (!histFacade || displayDims.empty()) {
        return std::make_shared<HistNullCharter>();
    }
    if (2 == displayDims.size()) {
        return std::make_shared<Hist2DFacadeCharter>(
                histFacade,
                std::array<int, 2>{{ displayDims[0], displayDims[1] }},
                paintDevice);
    }
    if (1 == displayDims.size()) {
        return std::make_shared<Hist1DFacadeCharter>(
                histFacade, displayDims[0], paintDevice);
    }
    assert(false);
}

/**
 * @brief Hist2DFacadeCharter::Hist2DFacadeCharter
 * @param histFacade
 * @param displayDims
 */
Hist2DFacadeCharter::Hist2DFacadeCharter(std::shared_ptr<const HistFacade> histFacade,
        std::array<int, 2> displayDims, QPaintDevice *paintDevice)
  : _histFacade(histFacade)
  , _displayDims(displayDims)
  , _paintDevice(paintDevice) {
    _histPainter = std::make_shared<Hist2DTexturePainter>();
    _histPainter->initialize();
    _histPainter->setTexture(_histFacade->texture(_displayDims));
    _histPainter->setColorMap(IHistPainter::YELLOW_BLUE);
    _histPainter->setRange(0.f, 1.f);
}

void Hist2DFacadeCharter::setSize(int width, int height, int devicePixelRatio) {
    _width = width;
    _height = height;
    _devicePixelRatio = devicePixelRatio;
}

void Hist2DFacadeCharter::setRange(float vMin, float vMax)
{
    _histPainter->setRange(vMin, vMax);
}

std::string Hist2DFacadeCharter::setMouseHover(float x, float y)
{
    auto hist2d = hist();
    float hx = x * devicePixelRatioF() - histLeft();
    float hy = histHeight() - (y * devicePixelRatioF() - histTop());
    if (hx < 0.f || hx >= histWidth() || hy < 0.f || hy >= histHeight()) {
        _hoveredBin[0] = -1;
        return "";
    }
    float dx = histWidth() / hist2d->dim()[0];
    float dy = histHeight() / hist2d->dim()[1];
    int ibx = hx / dx;
    int iby = hy / dy;
    _hoveredBin[0] = ibx;
    _hoveredBin[1] = iby;
    HistBin bin = hist2d->bin(ibx, iby);
    std::string valueStr = std::to_string(int(bin.value() + 0.5));
    std::string percentStr = std::to_string(int(bin.percent() * 100.f + 0.5f));
    return valueStr + " (" + percentStr + "%)";
}

void Hist2DFacadeCharter::chart() {
    // histogram
    if (_histPainter) {
        _histPainter->setRect(histLeft() / width(), histBottom() / height(),
                histWidth() / width(), histHeight() / height());
        _histPainter->paint();
    }
    // axes
    Painter painter(_paintDevice);
//    Painter painter =
//            _paintDevice ? Painter(_paintDevice) : Painter(width(), height());
    painter.setPen(QPen(Qt::black, 0.25f));
    auto hist2d = hist();
    float dx = histWidth() / hist2d->dim()[0];
    for (int ix = 0; ix <= hist2d->dim()[0]; ++ix) {
        painter.drawLine(
                QLineF(histLeft() + ix * dx, height() - histBottom(),
                    histLeft() + ix * dx, histTop()));
    }
    float dy = histHeight() / hist2d->dim()[1];
    for (int iy = 0; iy <= hist2d->dim()[1]; ++iy) {
        painter.drawLine(
                QLineF(histLeft(), height() - (histBottom() + iy * dy),
                    width() - histRight(),
                    height() - (histBottom() + iy * dy)));
    }
    // hovered bin
    if (-1 != _hoveredBin[0]) {
        painter.save();
        painter.setPen(QPen(QColor(231, 76, 60, 100), 4.f));
        painter.drawRect(histLeft() + _hoveredBin[0] * dx,
                histTop() + histHeight() - (_hoveredBin[1] + 1) * dy, dx, dy);
        painter.restore();
    }
    // ticks
    painter.setFont(QFont("mono", 8.f * devicePixelRatioF()));
    const float labelPixels = 28.f * devicePixelRatioF();
    int xFactor =
            std::max(
                1, int(hist2d->dim()[0] / histWidth() * labelPixels));
    for (int sx = 0; sx <= hist2d->dim()[0] / xFactor; ++sx) {
        int ix = sx * xFactor;
        painter.save();
        painter.translate(histLeft() + ix * dx,
                height() - (histBottom() - 2.f * devicePixelRatioF()));
        painter.rotate(-45);
        double min = hist2d->dimMin(0);
        double max = hist2d->dimMax(0);
        double number = float(ix) / hist2d->dim()[0] * (max - min) + min;
        painter.drawText(
                -50 * devicePixelRatio(), 0 * devicePixelRatio(),
                50 * devicePixelRatio(), 10 * devicePixelRatio(),
                Qt::AlignTop | Qt::AlignRight, QString::number(number, 'g', 3));
        painter.restore();
    }
    int yFactor =
            std::max(1,
                int(hist2d->dim()[1] / histHeight() * labelPixels));
    for (int sy = 0; sy <= hist2d->dim()[1] / yFactor; ++sy) {
        int iy = sy * yFactor;
        painter.save();
        painter.translate((histLeft() - 2.f * devicePixelRatioF()),
                height() - (histBottom() + iy * dy));
        painter.rotate(-45);
        double min = hist2d->dimMin(1);
        double max = hist2d->dimMax(1);
        double number = float(iy) / hist2d->dim()[1] * (max - min) + min;
        painter.drawText(
                -50 * devicePixelRatio(), -10 * devicePixelRatio(),
                50 * devicePixelRatio(), 10 * devicePixelRatio(),
                Qt::AlignBottom | Qt::AlignRight,
                QString::number(number, 'g', 3));
        painter.restore();
    }
    // labels
    painter.drawText(histLeft(), height() - labelBottom(),
            histWidth(), labelBottom(), Qt::AlignTop | Qt::AlignHCenter,
            QString::fromStdString(hist2d->var(0)));
    painter.save();
    painter.translate(0, height());
    painter.rotate(-90);
    painter.drawText(histBottom(), 0, histHeight(), labelLeft(),
            Qt::AlignBottom | Qt::AlignHCenter,
            QString::fromStdString(hist2d->var(1)));
    painter.restore();
//    painter.paint();
}

std::shared_ptr<const Hist> Hist2DFacadeCharter::hist() const {
    return _histFacade->hist(_displayDims);
}

/**
 * @brief Hist1DFacadeCharter::Hist1DFacadeCharter
 * @param histFacade
 * @param displayDim
 */
Hist1DFacadeCharter::Hist1DFacadeCharter(
        std::shared_ptr<const HistFacade> histFacade, int displayDim,
        QPaintDevice* paintDevice)
  : _histFacade(histFacade)
  , _displayDim(displayDim)
  , _paintDevice(paintDevice) {
    _histPainter = std::make_shared<Hist1DVBOPainter>();
    _histPainter->initialize();
    _histPainter->setVBO(_histFacade->vbo(_displayDim));
    _histPainter->setColorMap(IHistPainter::YELLOW_BLUE);
    _histPainter->setBackgroundColor({ 0.f, 0.f, 0.f, 0.f });
    _histPainter->setRange(0.f, 1.f);
}

void Hist1DFacadeCharter::setSize(int width, int height, int devicePixelRatio)
{
    _width = width;
    _height = height;
    _devicePixelRatio = devicePixelRatio;
}

void Hist1DFacadeCharter::setRange(float vMin, float vMax)
{
    _vMin = vMin;
    _vMax = vMax;
    _histPainter->setRange(vMin, vMax);
}

std::string Hist1DFacadeCharter::setMouseHover(float x, float /*y*/)
{
    auto hist = _histFacade->hist(_displayDim);
    float hx = x * devicePixelRatioF() - histLeft();
    if (hx < 0.f || hx >= histWidth()) {
        _hoveredBin = -1;
        return "";
    }
    float dx = histWidth() / hist->dim()[0];
    int ibx = hx / dx;
    _hoveredBin = ibx;
    HistBin bin = hist->bin(ibx);
    std::string valueStr = std::to_string(int(bin.value() + 0.5));
    std::string percentStr = std::to_string(int(bin.percent() * 100.f + 0.5f));
    return valueStr + " (" + percentStr + "%)";
}

void Hist1DFacadeCharter::chart()
{
    const float vMaxRatio = 0.9f;
    const int nTicks = 6;
    // ticks under the histogram
    {
//        Painter painter =
//                _paintDevice ?
//                    Painter(_paintDevice) : Painter(width(), height());
        Painter painter(_paintDevice);
        painter.setPen(QPen(Qt::black, 0.25f));
        for (auto i = 0; i < nTicks - 1; ++i) {
            float y = height() - histBottom()
                    - (i + 1) * vMaxRatio * histHeight() / (nTicks - 1);
            painter.drawLine(
                    QLineF(histLeft(), y, histLeft() + histWidth(), y));
        }
//        painter.paint();
    }
    // histogram
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (_histPainter) {
        _histPainter->setRect(histLeft() / width(), histBottom() / height(),
                histWidth() / width(), histHeight() / height());
        _histPainter->paint();
    }
    glDisable(GL_BLEND);
    // axes
//    Painter painter =
//            _paintDevice ? Painter(_paintDevice) : Painter(width(), height());
    Painter painter(_paintDevice);
    painter.setPen(QPen(Qt::black, 0.5f));
    auto hist = _histFacade->hist(_displayDim);
    painter.drawLine(
            QLineF(histLeft(), height() - histBottom(), histLeft(), histTop()));
    painter.drawLine(
            QLineF(histLeft(), height() - histBottom(),
                histLeft() + histWidth(), height() - histBottom()));
    // ticks
    auto font = QFont("mono", 8.f * devicePixelRatioF());
    auto fontMetrics = QFontMetricsF(font);
    painter.setFont(font);
    const float labelPixels = fontMetrics.height() * 3.f * devicePixelRatioF();
    float dx = histWidth() / hist->dim()[0];
    int xFactor =
            std::max(
                1, int(hist->dim()[0] / histWidth() * labelPixels));
    for (int sx = 0; sx <= hist->dim()[0] / xFactor; ++sx) {
        int ix = sx * xFactor;
        painter.save();
        painter.translate(histLeft() + ix * dx,
                height() - (histBottom() - 2.f * devicePixelRatioF()));
        painter.rotate(-45);
        double min = hist->dimMin(0);
        double max = hist->dimMax(0);
        double number = float(ix) / hist->dim()[0] * (max - min) + min;
        painter.drawText(
                -50 * devicePixelRatio(), 0 * devicePixelRatio(),
                50 * devicePixelRatio(), fontMetrics.height(),
                Qt::AlignTop | Qt::AlignRight, QString::number(number, 'g', 3));
        painter.restore();
    }
    float dy = vMaxRatio * histHeight() / nTicks;
    int yFactor =
            std::max(1, int(nTicks / (vMaxRatio * histHeight()) * labelPixels));
    for (int sy = 0; sy <= nTicks / yFactor; ++sy) {
        int iy = sy * yFactor;
        painter.save();
        painter.translate((histLeft() - 2.f * devicePixelRatioF()),
                height() - (histBottom() + iy * dy));
        painter.rotate(-45);
        double min = _vMin;
        double max = _vMax;
        double number = float(iy) / nTicks * (max - min) + min;
        painter.drawText(
                -50 * devicePixelRatio(), -10 * devicePixelRatio(),
                50 * devicePixelRatio(), fontMetrics.height(),
                Qt::AlignBottom | Qt::AlignRight,
                QString::number(number, 'g', 3));
        painter.restore();
    }
    // hovered bin
    if (-1 != _hoveredBin) {
        painter.fillRect(
                QRectF(
                    histLeft() + _hoveredBin * dx, histTop(), dx, histHeight()),
                QColor(231, 76, 60, 100));
    }
    // labels
    painter.drawText(histLeft(), height() - labelBottom(),
            histWidth(), labelBottom(), Qt::AlignTop | Qt::AlignHCenter,
            QString::fromStdString(hist->var(0)));
    painter.save();
    painter.translate(0, height());
    painter.rotate(-90);
    painter.drawText(histBottom(), 0, histHeight(), labelLeft(),
            Qt::AlignBottom | Qt::AlignHCenter, "frequency");
    painter.restore();
//    painter.paint(_paintDevice);
}
