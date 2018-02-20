#include "histcharter.h"
#include <cassert>
#include <histfacade.h>
#include <histpainter.h>
#include <histfacadepainter.h>
#include <painter.h>
#include <QMouseEvent>

namespace {

typedef PainterImpl UsePainterImpl;

template <typename T>
T clamp(const T& v, const T& a, const T& b) {
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
    _varRanges = varRanges;
}

void Hist2DFacadeCharter::setSelectedVarRanges(
        const IHistCharter::HistRangesMap &selectedVarRanges) {
    _selectedVarRanges.resize(2);
    for (int i = 0; i < _displayDims.size(); ++i) {
        _selectedVarRanges[i] = selectedVarRanges.at(_displayDims[i]);
    }
    _selectedBinBeg = {-1, -1};
    _selectedBinEnd = {-1, -1};
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
    _selectedVarRanges = binRangesToVarRanges(_selectedBinBeg, _selectedBinEnd);
}

void Hist2DFacadeCharter::mouseReleaseEvent(QMouseEvent *event) {
    _isMousePressed = false;
    _selectedVarRanges = binRangesToVarRanges(_selectedBinBeg, _selectedBinEnd);
    selectedHistRangesChanged({
        {_displayDims[0], _selectedVarRanges[0]},
        {_displayDims[1], _selectedVarRanges[1]}
    });
}

void Hist2DFacadeCharter::mouseMoveEvent(QMouseEvent *event) {
    float x = event->localPos().x();
    float y = event->localPos().y();
    auto hist2d = hist();
    auto binIds = posToBinIds(x, y);
    if (_isMousePressed && _isBinSelected) {
        _selectedBinEnd = binIds;
        _selectedVarRanges =
                binRangesToVarRanges(_selectedBinBeg, _selectedBinEnd);
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
    _hoveredBin = {-1, -1};
    _showLabel(0, 0, "");
}

void Hist2DFacadeCharter::chart(QPaintDevice *paintDevice) {
    // if there isn't enough space for labels
    if (histWidth() / chartWidth() < thresholdRatioToDrawLabels) {
        drawHist(chartLeft() / width(), chartBottom() / height(),
                chartWidth() / width(), chartHeight() / height());
        return;
    }
    // histogram
    drawHist(histLeft() / width(), histBottom() / height(),
            histWidth() / width(), histHeight() / height());
    // axes
    Painter painter(std::make_shared<UsePainterImpl>(), paintDevice);
    painter.setPen(QPen(Qt::black, 0.25f));
    auto hist2d = hist();
    auto xVarRange = _varRanges[0];
    double xvr = xVarRange[1] - xVarRange[0];
    auto yVarRange = _varRanges[1];
    double yvr = yVarRange[1] - yVarRange[0];
    auto xDimRange = _histFacade->dimRange(_displayDims[0]);
    double xdr = xDimRange[1] - xDimRange[0];
    auto yDimRange = _histFacade->dimRange(_displayDims[1]);
    double ydr = yDimRange[1] - yDimRange[0];
    double xBegBin = (xVarRange[0] - xDimRange[0]) / xdr * hist2d->dim()[0];
    double xEndBin = (xVarRange[1] - xDimRange[0]) / xdr * hist2d->dim()[0];
    double xNBins = xEndBin - xBegBin;
    double yBegBin = (yVarRange[0] - yDimRange[0]) / ydr * hist2d->dim()[1];
    double yEndBin = (yVarRange[1] - yDimRange[0]) / ydr * hist2d->dim()[1];
    double yNBins = yEndBin - yBegBin;
    double xOffset = (1.0 - std::fmod(xBegBin, 1.0)) / xNBins * histWidth();
    double yOffset = (1.0 - std::fmod(yBegBin, 1.0)) / yNBins * histHeight();
    float dx = histWidth() / xNBins;
    float dy = histHeight() / yNBins;
    painter.drawRect(QRectF(histLeft(), histTop(), histWidth(), histHeight()));
    if (dx > thresholdBinSizeToDrawGrid && dy > thresholdBinSizeToDrawGrid) {
        for (int ix = 0; ix < int(xNBins); ++ix) {
            painter.drawLine(
                    QLineF(
                        histLeft() + xOffset + ix * dx,
                        height() - histBottom(),
                        histLeft() + xOffset + ix * dx,
                        histTop()));
        }
        for (int iy = 0; iy < int(yNBins); ++iy) {
            painter.drawLine(
                    QLineF(
                        histLeft(),
                        height() - (histBottom() + yOffset + iy * dy),
                        width() - histRight(),
                        height() - (histBottom() + yOffset + iy * dy)));
        }
    }
    // selected bins
    if (isVarRangesSelected()) {
        std::array<std::array<double, 2>, 2> drawRanges;
        for (int iDim = 0; iDim < 2; ++iDim) {
            auto varRange = _varRanges[iDim];
            auto selVarRange = _selectedVarRanges[iDim];
            auto vr = varRange[1] - varRange[0];
            double lower = clamp((selVarRange[0] - varRange[0]) / vr, 0.0, 1.0);
            double upper = clamp((selVarRange[1] - varRange[0]) / vr, 0.0, 1.0);
            drawRanges[iDim] = {lower, upper};
        }
        painter.fillRect(
                QRectF(
                    histLeft() + drawRanges[0][0] * histWidth(),
                    histTop() + (1.0 - drawRanges[1][1]) * histHeight(),
                    (drawRanges[0][1] - drawRanges[0][0]) * histWidth(),
                    (drawRanges[1][1] - drawRanges[1][0]) * histHeight()),
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
                1, int(xNBins / histWidth() * labelPixels));
    for (int sx = 0; sx <= int(xNBins / xFactor); ++sx) {
        int ix = sx * xFactor;
        painter.save();
        painter.translate(histLeft() + ix * dx,
                height() - (histBottom() - 2.f * devicePixelRatioF()));
        painter.rotate(tickAngleDegree());
        double min = _varRanges[0][0];
        double max = _varRanges[0][1];
        double number = float(ix) / xNBins * (max - min) + min;
        painter.drawText(
                -tickWidth(), 0, tickWidth(), tickHeight(),
                Qt::AlignTop | Qt::AlignRight, QString::number(number, 'g', 3));
        painter.restore();
    }
    int yFactor =
            std::max(1,
                int(yNBins / histHeight() * labelPixels));
    for (int sy = 0; sy <= int(yNBins / yFactor); ++sy) {
        int iy = sy * yFactor;
        painter.save();
        painter.translate((histLeft() - 2.f * devicePixelRatioF()),
                height() - (histBottom() + iy * dy));
        painter.rotate(tickAngleDegree());
        double min = _varRanges[1][0];
        double max = _varRanges[1][1];
        double number = float(iy) / yNBins * (max - min) + min;
        painter.drawText(
                -tickWidth(), -tickHeight(), tickWidth(), tickHeight(),
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

std::array<double, 4> Hist2DFacadeCharter::normalizedBox() const {
    std::array<double, 4> box;
    {
        auto varRange = _varRanges[0];
        auto dimRange = _histFacade->dimRange(_displayDims[0]);
        box[0] = (varRange[0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
        box[2] = (varRange[1] - varRange[0]) / (dimRange[1] - dimRange[0]);
    }
    {
        auto varRange = _varRanges[1];
        auto dimRange = _histFacade->dimRange(_displayDims[1]);
        box[1] = (varRange[0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
        box[3] = (varRange[1] - varRange[0]) / (dimRange[1] - dimRange[0]);
    }
    return box;
}

void Hist2DFacadeCharter::drawHist(
        float top, float bottom, float width, float height) const {
    if (!_histPainter)
        return;
    _histPainter->setNormalizedViewport(top, bottom, width, height);
    auto box = normalizedBox();
    _histPainter->setNormalizedBox(box[0], box[1], box[2], box[3]);
    _histPainter->paint();
}

std::vector<std::array<double, 2>> Hist2DFacadeCharter::binRangesToVarRanges(
        const std::array<int, 2> &binBeg,
        const std::array<int, 2> &binEnd) const {
    auto hist2d = hist();
    auto dims = hist2d->dim();
    std::array<int, 2> binLower, binUpper;
    binLower[0] = clamp(std::min(binBeg[0], binEnd[0]), 0, dims[0] - 1);
    binLower[1] = clamp(std::min(binBeg[1], binEnd[1]), 0, dims[1] - 1);
    binUpper[0] = clamp(std::max(binBeg[0], binEnd[0]), 0, dims[0] - 1);
    binUpper[1] = clamp(std::max(binBeg[1], binEnd[1]), 0, dims[1] - 1);
    auto xFullRange = _histFacade->dimRange(0);
    auto yFullRange = _histFacade->dimRange(1);
    auto xBinWidth = (xFullRange[1] - xFullRange[0]) / dims[0];
    auto yBinWidth = (yFullRange[1] - yFullRange[0]) / dims[1];
    std::vector<std::array<double, 2>> varRanges(2);
    varRanges[0][0] = xFullRange[0] + xBinWidth * binLower[0];
    varRanges[0][1] = xFullRange[0] + xBinWidth * (1 + binUpper[0]);
    varRanges[1][0] = yFullRange[0] + yBinWidth * binLower[1];
    varRanges[1][1] = yFullRange[0] + yBinWidth * (1 + binUpper[1]);
    return varRanges;
}

bool Hist2DFacadeCharter::isVarRangesSelected() const {
    return 2 == _selectedVarRanges.size()
            && !std::isnan(_selectedVarRanges[0][0])
            && !std::isnan(_selectedVarRanges[0][1])
            && !std::isnan(_selectedVarRanges[1][0])
            && !std::isnan(_selectedVarRanges[1][1]);
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
    _varRange = varRanges[0];
}

void Hist1DFacadeCharter::setSelectedVarRanges(
        const HistRangesMap& varRangesMap) {
    _selectedVarRange = varRangesMap.at(_displayDim);
    _selectedBinRange = {-1, -1};
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
    _selectedBinRange[0] = ibx;
    _selectedBinRange[1] = ibx;
    _selectedVarRange = binRangeToVarRange(_selectedBinRange);
}

void Hist1DFacadeCharter::mouseReleaseEvent(QMouseEvent *event) {
    _isMousePressed = false;
    int ibx = posToBinId(event->localPos().x());
    _selectedBinRange[1] =
            clamp(ibx, 0, _histFacade->hist(_displayDim)->nBins() - 1);
    _selectedVarRange = binRangeToVarRange(_selectedBinRange);
    selectedHistRangesChanged({{_displayDim, _selectedVarRange}});
}

void Hist1DFacadeCharter::mouseMoveEvent(QMouseEvent *event) {
    float x = event->localPos().x();
    float y = event->localPos().y();
    auto hist = _histFacade->hist(_displayDim);
    int ibx = posToBinId(x);
    _hoveredBin = ibx;
    if (_isMousePressed && _isBinSelected) {
        _selectedBinRange[1] = clamp(ibx, 0, hist->nBins() - 1);
        _selectedVarRange = binRangeToVarRange(_selectedBinRange);
    }
    if (ibx < 0 || ibx >= hist->nBins()) {
        _showLabel(x, y, "");
        return;
    }
    double binFreq = hist->binFreq(ibx);
    float binPercent = hist->binPercent(ibx);
    std::vector<std::array<double, 2>> binRanges = hist->binRanges(ibx);
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
    auto varRange = _varRange;
    auto dimRange = _histFacade->dimRange(_displayDim);
    rect[0] = (varRange[0] - dimRange[0]) / (dimRange[1] - dimRange[0]);
    rect[2] = (varRange[1] - varRange[0]) / (dimRange[1] - dimRange[0]);
    return rect;
}

std::array<double, 2> Hist1DFacadeCharter::binRangeToVarRange(
        const std::array<int, 2> &binRange) {
    auto dimRange = _histFacade->dimRange(0);
    auto nBins = _histFacade->hist(_displayDim)->dim()[0];
    auto binWidth = (dimRange[1] - dimRange[0]) / nBins;
    int binLower = std::min(binRange[0], binRange[1]);
    int binUpper = std::max(binRange[0], binRange[1]);
    return {
        dimRange[0] + binWidth * binLower,
        dimRange[0] + binWidth * (1 + binUpper)
    };
}

bool Hist1DFacadeCharter::isVarRangeSelected() const {
    return !std::isnan(_selectedVarRange[0])
            && !std::isnan(_selectedVarRange[1]);
}

void Hist1DFacadeCharter::chart(QPaintDevice *paintDevice)
{
    const float vMaxRatio = 1.f / 1.1f;
    const int nTicks = 6;
    Painter painter(std::make_shared<UsePainterImpl>(), paintDevice);
    // if there isn't enough space for labels
    if (histWidth() / chartWidth() < thresholdRatioToDrawLabels) {
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
        double min = _varRange[0];
        double max = _varRange[1];
        double number = float(ix) / hist->dim()[0] * (max - min) + min;
        painter.drawText(
                -tickWidth(), 0, tickWidth(), tickHeight(),
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
                Qt::AlignBottom | Qt::AlignRight,
                QString::number(number, 'g', 3));
        painter.restore();
    }
    // selected bin
    if (isVarRangeSelected()) {
        double vr = _varRange[1] - _varRange[0];
        double lower =
                clamp((_selectedVarRange[0] - _varRange[0]) / vr, 0.0, 1.0);
        double upper =
                clamp((_selectedVarRange[1] - _varRange[0]) / vr, 0.0, 1.0);
        painter.fillRect(
                QRectF(
                    histLeft() + lower * histWidth(), histTop(),
                    (upper - lower) * histWidth(), histHeight()),
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
    if (2 == displayDims.size()) {
        Hist2DFacadeCharter* charter =
                static_cast<Hist2DFacadeCharter*>(_charter.get());
        charter->setDrawColormap(false);
    }
}

void HistFacadeCharter::setDrawColormap(bool drawColormap) {
    _drawColormap = drawColormap;
    Hist2DFacadeCharter* charter =
            dynamic_cast<Hist2DFacadeCharter*>(_charter.get());
    if (charter) {
        charter->setDrawColormap(false);
    }
}
