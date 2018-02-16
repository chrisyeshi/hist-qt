#include "histcharter.h"
#include <cassert>
#include <histfacade.h>
#include <histpainter.h>
#include <histfacadepainter.h>
#include <painter.h>
#include <QMouseEvent>

namespace {

typedef PainterImpl UsePainterImpl;

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
        std::vector<int> displayDims,
        std::function<void(float, float, const std::string &)> showLabel) {
    if (!histFacade || displayDims.empty()) {
        return std::make_shared<HistNullCharter>();
    }
    if (2 == displayDims.size()) {
        return std::make_shared<Hist2DFacadeCharter>(
                histFacade,
                std::array<int, 2>{{ displayDims[0], displayDims[1] }},
                showLabel);
    }
    if (1 == displayDims.size()) {
        return std::make_shared<Hist1DFacadeCharter>(
                histFacade, displayDims[0], showLabel);
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
        std::array<int, 2> displayDims,
        std::function<void(float, float, const std::string &)> showLabel)
  : _histFacade(histFacade)
  , _displayDims(displayDims)
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

void Hist2DFacadeCharter::setNormalizedViewport(
        float left, float top, float width, float height) {
    _viewport.setRect(left, top, width, height);
}

void Hist2DFacadeCharter::setFreqRange(float vMin, float vMax)
{
    _freqRange[0] = vMin;
    _freqRange[1] = vMax;
    _histPainter->setFreqRange(vMin, vMax);
}

void Hist2DFacadeCharter::setRanges(
        std::vector<std::array<double, 2> > varRanges) {
    /// TODO:
}

void Hist2DFacadeCharter::mousePressEvent(QMouseEvent *event) {
    _isMousePressed = true;
    auto binIds = posToBinIds(event->localPos().x(), event->localPos().y());
    if (binIds[0] < 0) {
        _isBinSelected = false;
        return;
    }
    _isBinSelected = true;
    _selectedBinBeg = binIds;
    _selectedBinEnd = binIds;
}

void Hist2DFacadeCharter::mouseReleaseEvent(QMouseEvent *event) {
    _isMousePressed = false;
    auto hist2d = hist();
    std::array<int, 2> binLower, binUpper;
    binLower[0] =
            clamp(std::min(_selectedBinBeg[0], _selectedBinEnd[0]),
                0, hist2d->dim()[0] - 1);
    binLower[1] =
            clamp(std::min(_selectedBinBeg[1], _selectedBinEnd[1]),
                0, hist2d->dim()[1] - 1);
    binUpper[0] =
            clamp(std::max(_selectedBinBeg[0], _selectedBinEnd[0]),
                0, hist2d->dim()[0] - 1);
    binUpper[1] =
            clamp(std::max(_selectedBinBeg[1], _selectedBinEnd[1]),
                0, hist2d->dim()[1] - 1);
    auto xFullRange = _histFacade->dimRange(0);
    auto yFullRange = _histFacade->dimRange(1);
    auto xBinWidth = (xFullRange[1] - xFullRange[0]) / hist()->dim()[0];
    auto yBinWidth = (yFullRange[1] - yFullRange[0]) / hist()->dim()[1];
    std::array<float, 2> xRange, yRange;
    xRange[0] = xFullRange[0] + xBinWidth * binLower[0];
    xRange[1] = xFullRange[0] + xBinWidth * (1 + binUpper[0]);
    yRange[0] = yFullRange[0] + yBinWidth * binLower[1];
    yRange[1] = yFullRange[0] + yBinWidth * (1 + binUpper[1]);
    selectedHistRangesChanged(
            {{_displayDims[0], xRange}, {_displayDims[1], yRange}});
}

void Hist2DFacadeCharter::mouseMoveEvent(QMouseEvent *event) {
    float x = event->localPos().x();
    float y = event->localPos().y();
    auto hist2d = hist();
    auto binIds = posToBinIds(x, y);
    if (_isMousePressed && _isBinSelected) {
        _selectedBinEnd = binIds;
    }
    if (binIds[0] < 0 || binIds[0] >= hist2d->dim()[0] ||
            binIds[1] < 0 || binIds[1] >= hist2d->dim()[1]) {
        _hoveredBin = {{-1, -1}};
        _showLabel(x, y, "");
        return;
    }
    _hoveredBin = binIds;
    int ibx = _hoveredBin[0];
    int iby = _hoveredBin[1];
    double binFreq = hist2d->binFreq(ibx, iby);
    float binPercent = hist2d->binPercent(ibx, iby);
    std::vector<std::array<double, 2>> binRanges = hist2d->binRanges(ibx, iby);
    QString label =
            QString("frequency: %1 (%2\%)\n%3: [%4, %5]\n%6: [%7, %8]")
                .arg(QString::number(binFreq, 'g', 5))
                .arg(QString::number(binPercent * 100.f, 'g', 3))
                .arg(hist2d->var(0).c_str())
                .arg(QString::number(binRanges[0][0], 'g', 3))
                .arg(QString::number(binRanges[0][1], 'g', 3))
                .arg(hist2d->var(1).c_str())
                .arg(QString::number(binRanges[1][0], 'g', 3))
                .arg(QString::number(binRanges[1][1], 'g', 3));
    _showLabel(x, y, label.toStdString());
}

void Hist2DFacadeCharter::leaveEvent(QEvent *event) {
    _showLabel(0, 0, "");
}

void Hist2DFacadeCharter::chart(QPaintDevice *paintDevice) {
    // histogram
    if (_histPainter) {
        _histPainter->setNormalizedViewport(
                histLeft() / width(), histBottom() / height(),
                histWidth() / width(), histHeight() / height());
        _histPainter->paint();
    }
    // axes
    Painter painter(std::make_shared<UsePainterImpl>(), paintDevice);
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
    // selected bins
    if (_isBinSelected) {
        std::array<int, 2> binLower, binUpper;
        binLower[0] =
                clamp(std::min(_selectedBinBeg[0], _selectedBinEnd[0]),
                    0, hist2d->dim()[0] - 1);
        binLower[1] =
                clamp(std::min(_selectedBinBeg[1], _selectedBinEnd[1]),
                    0, hist2d->dim()[1] - 1);
        binUpper[0] =
                clamp(std::max(_selectedBinBeg[0], _selectedBinEnd[0]),
                    0, hist2d->dim()[0] - 1);
        binUpper[1] =
                clamp(std::max(_selectedBinBeg[1], _selectedBinEnd[1]),
                    0, hist2d->dim()[1] - 1);
        painter.fillRect(
                histLeft() + binLower[0] * dx,
                histTop() + histHeight() - (binUpper[1] + 1) * dy,
                (binUpper[0] - binLower[0] + 1) * dx,
                (binUpper[1] - binLower[1] + 1) * dy,
                QColor(231, 76, 60, 100));
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
    if (_drawColormap) {
        // colormap
        QLinearGradient colormap(colormapLeft(), colormapTop(),
                colormapLeft() + colormapWidth(), colormapTop());
        std::vector<glm::vec3> yellowBlue = ColormapPresets::yellowBlue();
        for (auto iColor = 0; iColor < yellowBlue.size(); ++iColor) {
            auto colorVector = yellowBlue[iColor];
            QColor color(colorVector.x * 255, colorVector.y * 255,
                    colorVector.z * 255);
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
                colormapWidth(), fontMetrics.height(),
                Qt::AlignTop | Qt::AlignLeft,
                QString::number(_freqRange[0], 'g', 3));
        painter.drawText(
                colormapLeft(),
                height() - colormapBottom() + colormapBottomPadding(),
                colormapWidth(), fontMetrics.height(),
                Qt::AlignTop | Qt::AlignRight,
                QString::number(_freqRange[1], 'g', 3));
    }
}

std::shared_ptr<const Hist> Hist2DFacadeCharter::hist() const {
    return _histFacade->hist(_displayDims);
}

std::array<int, 2> Hist2DFacadeCharter::posToBinIds(float x, float y) const {
    float hx = x * devicePixelRatioF() - histLeft();
    float hy = histHeight() - (y * devicePixelRatioF() - histTop());
    auto hist2d = hist();
    float dx = histWidth() / hist2d->dim()[0];
    float dy = histHeight() / hist2d->dim()[1];
    int ibx = hx / dx;
    int iby = hy / dy;
    return {{ibx, iby}};
}

/**
 * @brief Hist1DFacadeCharter::Hist1DFacadeCharter
 * @param histFacade
 * @param displayDim
 */
Hist1DFacadeCharter::Hist1DFacadeCharter(
        std::shared_ptr<const HistFacade> histFacade, int displayDim,
        std::function<void(float, float, const std::string &)> showLabel)
  : _histFacade(histFacade)
  , _displayDim(displayDim)
  , _showLabel(showLabel) {
    _histPainter = std::make_shared<Hist1DVBOPainter>();
    _histPainter->initialize();
    _histPainter->setVBO(_histFacade->vbo(_displayDim));
    _histPainter->setColorMap(IHistPainter::YELLOW_BLUE);
    _histPainter->setBackgroundColor({ 0.f, 0.f, 0.f, 0.f });
    _histPainter->setFreqRange(0.f, 1.f);
}

void Hist1DFacadeCharter::setSize(int width, int height, int devicePixelRatio) {
    _width = width;
    _height = height;
    _devicePixelRatio = devicePixelRatio;
}

void Hist1DFacadeCharter::setNormalizedViewport(
        float left, float top, float width, float height) {
    _viewport.setRect(left, top, width, height);
}

void Hist1DFacadeCharter::setFreqRange(float vMin, float vMax)
{
    _vMin = vMin;
    _vMax = vMax;
    _histPainter->setFreqRange(vMin, vMax);
}

void Hist1DFacadeCharter::setRanges(
        std::vector<std::array<double, 2>> varRanges) {
    _varRanges = varRanges;
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
    std::vector<std::array<double, 2>> binRanges = hist->binRanges({ibx});
    QString label =
            QString("frequency: %1 (%2\%)\n%3: [%4, %5]")
                .arg(QString::number(binFreq, 'g', 5))
                .arg(QString::number(binPercent * 100.f, 'g', 3))
                .arg(hist->var(0).c_str())
                .arg(QString::number(binRanges[0][0], 'g', 3))
                .arg(QString::number(binRanges[0][1], 'g', 3));
    _showLabel(x, y, label.toStdString());
}

void Hist1DFacadeCharter::leaveEvent(QEvent*) {
    _hoveredBin = -1;
    _showLabel(0, 0, "");
}

int Hist1DFacadeCharter::posToBinId(float x) const {
    float hx = x * devicePixelRatioF() - histLeft();
    float dx = histWidth() / _histFacade->hist(_displayDim)->dim()[0];
    int ibx = hx / dx;
    return ibx;
}

void Hist1DFacadeCharter::drawHist(
        double left, double bottom, double width, double height) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (_histPainter) {
        _histPainter->setNormalizedViewport(left, bottom, width, height);
        auto box = normalizedBox();
        _histPainter->setNormalizedBox(box[0], box[1], box[2], box[3]);
        _histPainter->paint();
    }
    glDisable(GL_BLEND);
}

std::array<double, 4> Hist1DFacadeCharter::normalizedBox() const {
    std::array<double, 4> rect = {0.0, 0.0, 1.0, 1.0};
    auto varRange = _varRanges[0];
    auto dimRange = _histFacade->dimRange(_displayDim);
    rect[0] = (varRange[0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
    rect[2] = (varRange[1] - varRange[0]) / (dimRange[1] - dimRange[0]);
    return rect;
}

void Hist1DFacadeCharter::chart(QPaintDevice *paintDevice)
{
    const float vMaxRatio = 1.f / 1.1f;
    const int nTicks = 6;
    Painter painter(std::make_shared<UsePainterImpl>(), paintDevice);
//    // if there isn't enough space for anything
//    if (chartWidth() < 50.f) {
//        painter.fillRect(
//                QRectF(chartLeft(), chartTop(), chartWidth(), chartHeight()),
//                Qt::red);
//        return;
//    }
    // if there isn't enough space for labels
    if (histWidth() / chartWidth() < 0.5f) {
        drawHist(chartLeft() / width(), chartBottom() / height(),
                chartWidth() / width(), chartHeight() / height());
        return;
    }
    // draw all the labels
    // ticks under the histogram
    painter.save();
    painter.setPen(QPen(Qt::black, 0.25f));
    for (auto i = 0; i < nTicks - 1; ++i) {
        float y = height() - histBottom()
                - (i + 1) * vMaxRatio * histHeight() / (nTicks - 1);
        painter.drawLine(
                QLineF(histLeft(), y, histLeft() + histWidth(), y));
    }
    painter.restore();
    // histogram
    drawHist(histLeft() / width(), histBottom() / height(),
            histWidth() / width(), histHeight() / height());
    // axes
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
        double min = _varRanges[0][0];
        double max = _varRanges[0][1];
//        double min = hist->dimMin(0);
//        double max = hist->dimMax(0);
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

/**
 * @brief HistFacadeCharter::HistFacadeCharter
 */
HistFacadeCharter::HistFacadeCharter() : _charter(IHistCharter::create()) {

}

void HistFacadeCharter::chart(QPaintDevice *paintDevice) {
    if (_viewport[0] > 1.f || _viewport[1] > 1.f
            || _viewport[0] + _viewport[2] < 0.f
            || _viewport[1] + _viewport[3] < 0.f) {
        return;
    }
    _charter->setSize(_width, _height, 1);
    _charter->setFreqRange(_freqRange[0], _freqRange[1]);
    _charter->setRanges(_varRanges);
    _charter->setNormalizedViewport(
            _viewport[0], _viewport[1], _viewport[2], _viewport[3]);
    _charter->chart(paintDevice);
}

void HistFacadeCharter::setSize(float width, float height) {
    _width = width;
    _height = height;
}

void HistFacadeCharter::setNormalizedViewport(
        float left, float top, float width, float height) {
    _viewport[0] = left;
    _viewport[1] = top;
    _viewport[2] = width;
    _viewport[3] = height;
}

void HistFacadeCharter::setFreqRange(float vMin, float vMax) {
    _freqRange[0] = vMin;
    _freqRange[1] = vMax;
}

void HistFacadeCharter::setRanges(std::vector<std::array<double, 2>> ranges) {
    _varRanges = ranges;
}

void HistFacadeCharter::setHist(
        std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    _charter = IHistCharter::create(histFacade, displayDims);
}
