#include "histcharter.h"
#include <cassert>
#include <histfacade.h>
#include <histpainter.h>
#include <painter.h>
#include <QMouseEvent>

namespace {

typedef PainterYYGLImpl UsePainterImpl;

int clamp(int v, int a, int b) {
    return std::max(a, std::min(b, v));
}

} // unnamed namespace

/**
 * @brief IHistCharter::create
 * @param histFacade
 * @param displayDims
 * @return
 */
std::shared_ptr<IHistCharter> IHistCharter::create(
        std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims, QPaintDevice *paintDevice,
        std::function<void(float, float, const std::string &)> showLabel) {
    if (!histFacade || displayDims.empty()) {
        return std::make_shared<HistNullCharter>();
    }
    if (2 == displayDims.size()) {
        return std::make_shared<Hist2DFacadeCharter>(
                histFacade,
                std::array<int, 2>{{ displayDims[0], displayDims[1] }},
                paintDevice, showLabel);
    }
    if (1 == displayDims.size()) {
        return std::make_shared<Hist1DFacadeCharter>(
                histFacade, displayDims[0], paintDevice, showLabel);
    }
    assert(false);
}

/**
 * @brief Hist2DFacadeCharter::Hist2DFacadeCharter
 * @param histFacade
 * @param displayDims
 */
Hist2DFacadeCharter::Hist2DFacadeCharter(
        std::shared_ptr<const HistFacade> histFacade,
        std::array<int, 2> displayDims, QPaintDevice *paintDevice,
        std::function<void(float, float, const std::string &)> showLabel)
  : _histFacade(histFacade)
  , _displayDims(displayDims)
  , _paintDevice(paintDevice)
  , _showLabel(showLabel) {
    _histPainter = std::make_shared<Hist2DTexturePainter>();
    _histPainter->initialize();
    _histPainter->setTexture(_histFacade->texture(_displayDims));
    _histPainter->setColorMap(IHistPainter::YELLOW_BLUE);
    _histPainter->setFreqRange(_freqRange[0], _freqRange[1]);
}

void Hist2DFacadeCharter::setSize(int width, int height, int devicePixelRatio) {
    _width = width;
    _height = height;
    _devicePixelRatio = devicePixelRatio;
}

void Hist2DFacadeCharter::setRange(float vMin, float vMax)
{
    _freqRange[0] = vMin;
    _freqRange[1] = vMax;
    _histPainter->setFreqRange(vMin, vMax);
}

void Hist2DFacadeCharter::mousePressEvent(QMouseEvent *event) {

}

void Hist2DFacadeCharter::mouseReleaseEvent(QMouseEvent *event) {

}

void Hist2DFacadeCharter::mouseMoveEvent(QMouseEvent *event) {
    float x = event->localPos().x();
    float y = event->localPos().y();
    auto hist2d = hist();
    float hx = x * devicePixelRatioF() - histLeft();
    float hy = histHeight() - (y * devicePixelRatioF() - histTop());
    if (hx < 0.f || hx >= histWidth() || hy < 0.f || hy >= histHeight()) {
        _hoveredBin[0] = -1;
        _showLabel(x, y, "");
        return;
    }
    float dx = histWidth() / hist2d->dim()[0];
    float dy = histHeight() / hist2d->dim()[1];
    int ibx = hx / dx;
    int iby = hy / dy;
    _hoveredBin[0] = ibx;
    _hoveredBin[1] = iby;
    double binFreq = hist2d->binFreq(ibx, iby);
    float binPercent = hist2d->binPercent(ibx, iby);
    std::string valueStr = std::to_string(int(binFreq + 0.5));
    std::string percentStr = std::to_string(int(binPercent * 100.f + 0.5f));
    _showLabel(x, y, valueStr + " (" + percentStr + "%)");
}

void Hist2DFacadeCharter::leaveEvent(QEvent *event) {
    _showLabel(0, 0, "");
}

void Hist2DFacadeCharter::chart() {
    // histogram
    if (_histPainter) {
        _histPainter->setNormalizedViewportAndRect(
                histLeft() / width(), histBottom() / height(),
                histWidth() / width(), histHeight() / height());
        _histPainter->paint();
    }
    // axes
    Painter painter(std::make_shared<UsePainterImpl>(), _paintDevice);
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
    auto font = QFont("mono", 8.f * devicePixelRatioF());
    auto fontMetrics = QFontMetricsF(font);
    painter.setFont(font);
    const float labelPixels = fontMetrics.height() * 3.f * devicePixelRatioF();
    int xFactor =
            std::max(
                1, int(hist2d->dim()[0] / histWidth() * labelPixels));
    for (int sx = 0; sx <= hist2d->dim()[0] / xFactor; ++sx) {
        int ix = sx * xFactor;
        painter.save();
        painter.translate(histLeft() + ix * dx,
                height() - (histBottom() - 2.f * devicePixelRatioF()));
        painter.rotate(tickAngleDegree());
        double min = hist2d->dimMin(0);
        double max = hist2d->dimMax(0);
        double number = float(ix) / hist2d->dim()[0] * (max - min) + min;
        painter.drawText(
                -tickWidth(), 0, tickWidth(), tickHeight(),
//                -50 * devicePixelRatio(), 0 * devicePixelRatio(),
//                50 * devicePixelRatio(), fontMetrics.height(),
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
        painter.rotate(tickAngleDegree());
        double min = hist2d->dimMin(1);
        double max = hist2d->dimMax(1);
        double number = float(iy) / hist2d->dim()[1] * (max - min) + min;
        painter.drawText(
                -tickWidth(), -tickHeight(), tickWidth(), tickHeight(),
//                -50 * devicePixelRatio(), -10 * devicePixelRatio(),
//                50 * devicePixelRatio(), fontMetrics.height(),
                Qt::AlignBottom | Qt::AlignRight,
                QString::number(number, 'g', 3));
        painter.restore();
    }
    // labels
    painter.drawText(
            histLeft(), height() - labelBottom() - fontMetrics.height(),
            histWidth(), fontMetrics.height(), Qt::AlignTop | Qt::AlignHCenter,
            QString::fromStdString(hist2d->var(0)));
    painter.save();
    painter.translate(0, height());
    painter.rotate(-90);
    painter.drawText(histBottom(), 0, histHeight(), labelLeft(),
            Qt::AlignBottom | Qt::AlignHCenter,
            QString::fromStdString(hist2d->var(1)));
    painter.restore();
    // colormap
    QLinearGradient colormap(colormapLeft(), colormapTop(),
            colormapLeft() + colormapWidth(), colormapTop());
    std::vector<glm::vec3> yellowBlue = ColormapPresets::yellowBlue();
    for (auto iColor = 0; iColor < yellowBlue.size(); ++iColor) {
        auto colorVector = yellowBlue[iColor];
        QColor color(
                colorVector.x * 255, colorVector.y * 255, colorVector.z * 255);
        colormap.setColorAt(iColor / float(yellowBlue.size() - 1), color);
    }
    painter.fillRect(colormapLeft(), colormapTop(), colormapWidth(),
            colormapHeight(), colormap);
    painter.drawRect(colormapLeft(), colormapTop(), colormapWidth(),
            colormapHeight());
    // colormap labels
    painter.drawText(
            colormapLeft(),
            height() - colormapBottom() + colormapBottomPadding(),
            colormapWidth(), fontMetrics.height(),
            Qt::AlignTop | Qt::AlignHCenter, QString("frequency"));
    painter.drawText(
            colormapLeft(),
            height() - colormapBottom() + colormapBottomPadding(),
            colormapWidth(), fontMetrics.height(), Qt::AlignTop | Qt::AlignLeft,
            QString::number(_freqRange[0], 'g', 3));
    painter.drawText(
            colormapLeft(),
            height() - colormapBottom() + colormapBottomPadding(),
            colormapWidth(), fontMetrics.height(),
            Qt::AlignTop | Qt::AlignRight,
            QString::number(_freqRange[1], 'g', 3));
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
        QPaintDevice* paintDevice,
        std::function<void(float, float, const std::string &)> showLabel)
  : _histFacade(histFacade)
  , _displayDim(displayDim)
  , _paintDevice(paintDevice)
  , _showLabel(showLabel) {
    _histPainter = std::make_shared<Hist1DVBOPainter>();
    _histPainter->initialize();
    _histPainter->setVBO(_histFacade->vbo(_displayDim));
    _histPainter->setColorMap(IHistPainter::YELLOW_BLUE);
    _histPainter->setBackgroundColor({ 0.f, 0.f, 0.f, 0.f });
    _histPainter->setFreqRange(0.f, 1.f);
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
    _histPainter->setFreqRange(vMin, vMax);
}

void Hist1DFacadeCharter::mousePressEvent(QMouseEvent *event) {
    _isMousePressed = true;
    auto hist = _histFacade->hist(_displayDim);
    int ibx = posToBinId(event->localPos().x());
    if (ibx < 0 || ibx >= hist->nBins()) {
        _isBinSelected = false;
        return;
    }
    _isBinSelected = true;
    _selectedBinBeg = ibx;
    _selectedBinEnd = ibx;
}

void Hist1DFacadeCharter::mouseReleaseEvent(QMouseEvent *event) {
    _isMousePressed = false;
    auto dimRange = _histFacade->dimRange(0);
    auto nBins = _histFacade->hist(_displayDim)->dim()[0];
    auto binWidth = (dimRange[1] - dimRange[0]) / nBins;
    int binLower = std::min(_selectedBinBeg, _selectedBinEnd);
    int binUpper = std::max(_selectedBinBeg, _selectedBinEnd);
    float xMin = dimRange[0] + binWidth * binLower;
    float xMax = dimRange[0] + binWidth * (1 + binUpper);
    selectedHistRangesChanged({{_displayDim, {xMin, xMax}}});
}

void Hist1DFacadeCharter::mouseMoveEvent(QMouseEvent *event) {
    float x = event->localPos().x();
    float y = event->localPos().y();
    auto hist = _histFacade->hist(_displayDim);
    int ibx = posToBinId(x);
    _hoveredBin = ibx;
    if (_isMousePressed && _isBinSelected) {
        _selectedBinEnd = clamp(ibx, 0, hist->nBins() - 1);
    }
    if (ibx < 0 || ibx >= hist->nBins()) {
        _showLabel(x, y, "");
        return;
    }
    double binFreq = hist->binFreq(ibx);
    float binPercent = hist->binPercent(ibx);
    std::string valueStr = std::to_string(int(binFreq + 0.5));
    std::string percentStr = std::to_string(int(binPercent * 100.f + 0.5f));
    _showLabel(x, y, valueStr + " (" + percentStr + "%)");
}

void Hist1DFacadeCharter::leaveEvent(QEvent*) {
    _showLabel(0, 0, "");
}

int Hist1DFacadeCharter::posToBinId(float x) const {
    float hx = x * devicePixelRatioF() - histLeft();
    float dx = histWidth() / _histFacade->hist(_displayDim)->dim()[0];
    int ibx = hx / dx;
    return ibx;
}

void Hist1DFacadeCharter::chart()
{
    const float vMaxRatio = 0.9f;
    const int nTicks = 6;
    // ticks under the histogram
    {
        Painter painter(std::make_shared<UsePainterImpl>(), _paintDevice);
        painter.setPen(QPen(Qt::black, 0.25f));
        for (auto i = 0; i < nTicks - 1; ++i) {
            float y = height() - histBottom()
                    - (i + 1) * vMaxRatio * histHeight() / (nTicks - 1);
            painter.drawLine(
                    QLineF(histLeft(), y, histLeft() + histWidth(), y));
        }
    }
    // histogram
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (_histPainter) {
        _histPainter->setNormalizedViewportAndRect(
                histLeft() / width(), histBottom() / height(),
                histWidth() / width(), histHeight() / height());
        _histPainter->paint();
    }
    glDisable(GL_BLEND);
    // axes
    Painter painter(std::make_shared<UsePainterImpl>(), _paintDevice);
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
        painter.rotate(tickAngleDegree());
        double min = hist->dimMin(0);
        double max = hist->dimMax(0);
        double number = float(ix) / hist->dim()[0] * (max - min) + min;
        painter.drawText(
                -tickWidth(), 0, tickWidth(), tickHeight(),
//                -50 * devicePixelRatio(), 0 * devicePixelRatio(),
//                50 * devicePixelRatio(), fontMetrics.height(),
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
        painter.rotate(tickAngleDegree());
        double min = _vMin;
        double max = _vMax;
        double number = float(iy) / nTicks * (max - min) + min;
        painter.drawText(
                -tickWidth(), -tickHeight(), tickWidth(), tickHeight(),
//                -50 * devicePixelRatio(), -10 * devicePixelRatio(),
//                50 * devicePixelRatio(), fontMetrics.height(),
                Qt::AlignBottom | Qt::AlignRight,
                QString::number(number, 'g', 3));
        painter.restore();
    }
    // selected bin
    if (_isBinSelected) {
        int binLower = std::min(_selectedBinBeg, _selectedBinEnd);
        int binUpper = std::max(_selectedBinBeg, _selectedBinEnd);
        painter.fillRect(
                QRectF(histLeft() + binLower * dx, histTop(),
                    (binUpper - binLower + 1) * dx, histHeight()),
                QColor(231, 76, 60, 100));
    }
    // hovered bin
    if (0 <= _hoveredBin &&
            _hoveredBin < _histFacade->hist(_displayDim)->dim()[0]) {
        painter.fillRect(
                QRectF(
                    histLeft() + _hoveredBin * dx, histTop(), dx, histHeight()),
                QColor(231, 76, 60, 100));
    }
    // labels
    painter.drawText(
            histLeft(), height() - labelBottom() - fontMetrics.height(),
            histWidth(), fontMetrics.height(), Qt::AlignTop | Qt::AlignHCenter,
            QString::fromStdString(hist->var(0)));
    painter.save();
    painter.translate(0, height());
    painter.rotate(-90);
    painter.drawText(histBottom(), 0, histHeight(), labelLeft(),
            Qt::AlignBottom | Qt::AlignHCenter, "frequency");
    painter.restore();
}
