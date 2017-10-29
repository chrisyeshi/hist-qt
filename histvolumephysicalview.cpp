#include "histvolumephysicalview.h"
#include <QBoxLayout>
#include <QLabel>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <histview.h>
#include <lazyui.h>
#include <histfacadepainter.h>

namespace {

float calcAverage(std::shared_ptr<const Hist1D> hist) {
    double sum = 0.f;
    for (int iBin = 0; iBin < hist->nBins(); ++iBin) {
        double deltaBinValue =
                (hist->dimMax(0) - hist->dimMin(0)) / hist->nBins();
        double beginBinValue = 0.5 * deltaBinValue;
        double binValue = beginBinValue + iBin * deltaBinValue;
        double frequency = hist->bin(iBin).value();
        sum += binValue * frequency;
    }
    return sum / hist->nBins();
}

QStringList getHistConfigNames(const std::vector<HistConfig>& configs) {
    QStringList names;
    names.reserve(configs.size());
    for (unsigned int iConfig = 0; iConfig < configs.size(); ++iConfig) {
        names.insert(iConfig, QString::fromStdString(configs[iConfig].name()));
    }
    return names;
}

QMap<QString, std::vector<int>> getVarsToDims(const HistConfig& config) {
    QMap<QString, std::vector<int>> varsToDims;
    for (int i = 0; i < int(config.vars.size()); ++i) {
        QString str = QString::fromStdString(config.vars[i]);
        varsToDims.insert(str, {i});
    }
    for (int i = 0; i < int(config.vars.size()); ++i)
    for (int j = i + 1; j < int(config.vars.size()); ++j) {
        QString str =
                QString::fromStdString(config.vars[i] + "-" + config.vars[j]);
        varsToDims.insert(str, {i});
    }
    return varsToDims;
}

QStringList getHistDimVars(const HistConfig& config) {
    QStringList vars;
    for (int i = 0; i < int(config.vars.size()); ++i) {
        QString str = QString::fromStdString(config.vars[i]);
        vars.append(str);
    }
    for (int i = 0; i < int(config.vars.size()); ++i)
    for (int j = i + 1; j < int(config.vars.size()); ++j) {
        QString str =
                QString::fromStdString(config.vars[i] + "-" + config.vars[j]);
        vars.append(str);
    }
    return vars;
}

} // unnamed namespace

/**
 * @brief HistVolumePhysicalView::HistVolumePhysicalView
 * @param parent
 */
HistVolumePhysicalView::HistVolumePhysicalView(QWidget *parent)
      : HistVolumeView(parent)
      , _histVolumeView(new HistVolumePhysicalOpenGLView(this)) {
    auto hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(5);
    hLayout->addWidget(_histVolumeView, 3);
    connect(_histVolumeView,
            SIGNAL(
                selectedHistsChanged(std::vector<std::shared_ptr<const Hist>>)),
            this,
            SIGNAL(
                selectedHistsChanged(std::vector<std::shared_ptr<const Hist>>)));
}

void HistVolumePhysicalView::update() {
    _histVolumeView->setHistVolume(
            _histConfigs[_currHistConfigId],
            _dataStep->smartVolume(currHistName()));
    _histVolumeView->update();
}

void HistVolumePhysicalView::setHistConfigs(std::vector<HistConfig> configs) {
    _histConfigs = configs;
    _currHistConfigId = 0;
    _histDims = {0};
    LazyUI::instance().labeledCombo(
            tr("histVolume"), tr("Histogram Volumes"),
            getHistConfigNames(_histConfigs), this,
            [this](const QString& text) {
        QStringList names = getHistConfigNames(_histConfigs);
        assert(names.contains(text));
        _currHistConfigId = names.indexOf(text);
        update();
    });
}

void HistVolumePhysicalView::setDataStep(std::shared_ptr<DataStep> dataStep) {
    _dataStep = dataStep;
}

std::string HistVolumePhysicalView::currHistName() const {
    return _histConfigs[_currHistConfigId].name();
}

/**
 * @brief HistVolumePhysicalOpenGLView::HistVolumePhysicalOpenGLView
 * @param parent
 */
HistVolumePhysicalOpenGLView::HistVolumePhysicalOpenGLView(QWidget *parent)
      : OpenGLWidget(parent)
//      , _camera(std::make_shared<Camera>())
{
    setMouseTracking(true);
    delayForInit([this]() {
        LazyUI::instance().labeledCombo(
                tr("sliceDirections"), tr("Slicing Direction"),
                {tr("YZ"), tr("XZ"), tr("XY")}, this,
                [this](const QString& text) {
            if (tr("YZ") == text) {
                _currOrien = YZ;
            } else if (tr("XZ") == text) {
                _currOrien = XZ;
            } else if (tr("XY") == text) {
                _currOrien = XY;
            }
            _currSliceId = _defaultSliceId;
            updateSliceIdScrollBar();
            updateCurrSlice();
            update();
        });
    });

    delayForInit([this]() {
//        glClearColor(0.9f, 0.9f, 0.9f, 0.9f);
        glClearColor(0.8f, 0.8f, 0.8f, 0.8f);
//        _volren =
//                yy::volren::VolRenFactory::create(
//                    yy::volren::Method_Raycast_GL);
//        _volren->initializeGL();
    });
}

void HistVolumePhysicalOpenGLView::setHistVolume(
        HistConfig histConfig,
        std::shared_ptr<HistFacadeVolume> histVolume) {
    _histVolume = histVolume;
    _currDims = _defaultDims;
    _currSliceId = _defaultSliceId;
    LazyUI::instance().labeledCombo(
            tr("histVar"), tr("Histogram Variables"),
            getHistDimVars(histConfig), this, [=](const QString& text) {
        auto varsToDims = getVarsToDims(histConfig);
        std::vector<int> dims = varsToDims[text];
        assert(!dims.empty());
        _currDims = dims;
        updateSliceIdScrollBar();
        updateCurrSlice();
        update();
    });
    updateSliceIdScrollBar();
    delayForInit([this]() {
        updateCurrSlice();

//        int nVar = _histVolume->vars().size();
//        _avgVolumes.resize(nVar);
//        for (int iVar = 0; iVar < nVar; ++iVar) {
//            auto dimHists = _histVolume->dimHists();
//            auto nHist = dimHists[0] * dimHists[1] * dimHists[2];
//            std::unique_ptr<unsigned char[]> uniqueAvgBuffer(
//                    new unsigned char[nHist * sizeof(float)]);
//            float* avgBuffer = reinterpret_cast<float*>(uniqueAvgBuffer.get());
//            for (int z = 0; z < dimHists[2]; ++z)
//            for (int y = 0; y < dimHists[1]; ++y)
//            for (int x = 0; x < dimHists[0]; ++x) {
//                int iHist = dimHists.idstoflat(x, y, z);
//                avgBuffer[iHist] =
//                        calcAverage(
//                            std::static_pointer_cast<const Hist1D>(
//                                _histVolume->hist(iHist)->hist(iVar)));
//            }
//            std::shared_ptr<yy::Volume> volume =
//                    std::make_shared<yy::Volume>(
//                        uniqueAvgBuffer, yy::Volume::DT_Float,
//                        dimHists[0], dimHists[1], dimHists[2]);
//            _avgVolumes[iVar] = std::make_shared<yy::VolumeGL>(volume);
//        }
//        auto vol = _avgVolumes[_currVarId];
//        _volren->setVolume(vol);
//        _volren->setScalarRange(
//                vol->getStats().range.first, vol->getStats().range.second);
//        QVector3D bmin(0.f, 0.f, 0.f);
//        QVector3D bmax(vol->w() * vol->sx(), vol->h() * vol->sy(),
//                vol->d() * vol->sz());
//        _camera->reset(bmin, bmax);
    });
}

void HistVolumePhysicalOpenGLView::resizeGL(int w, int h) {
    if (!_histVolume) {
        return;
    }
    boundSliceTransform();
    updateHistPainterRects();
//    _camera->setAspectRatio(float(w) / float(h));
//    _volren->resize(w, h);
}

void HistVolumePhysicalOpenGLView::paintGL() {
    if (!_histVolume) {
        return;
    }
    for (auto painter : _histPainters) {
        painter->paint();
    }
    // hovered histogram
    if (isHistSliceIdsValid(_hoveredHistSliceIds)) {
        QPainter painter(this);
        painter.setPen(QPen(quarterColor(), 6.f * _histSpacing));
        painter.drawRect(calcHistRect(_hoveredHistSliceIds));
    }
    // selected histograms
    auto selectedHistSliceIds = filterByCurrSlice(_selectedHistIds);
    if (!selectedHistSliceIds.empty()) {
        QPainter painter(this);
        painter.setPen(QPen(fullColor(), 6.f * _histSpacing));
        for (auto histSliceIds : selectedHistSliceIds) {
            painter.drawRect(calcHistRect(histSliceIds));
        }
    }

//    _volren->setMatVP(_camera->matView(), _camera->matProj());
//    _volren->render();
//    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    _volren->output()->draw();
}

void HistVolumePhysicalOpenGLView::mousePressEvent(QMouseEvent *event) {
    _mousePress = event->localPos();
    _mousePrev = event->localPos();
}

void HistVolumePhysicalOpenGLView::mouseReleaseEvent(QMouseEvent *event) {
    auto mouseCurr = event->localPos();
    QVector2D mouseDelta(mouseCurr - _mousePress);
    if (mouseDelta.length() < _clickDelta) {
        mouseClickEvent(event);
    }
}

void HistVolumePhysicalOpenGLView::mouseClickEvent(QMouseEvent *event) {
    auto histSliceIds = localPositionToHistSliceId(event->localPos());
    if (Qt::NoModifier == event->modifiers()) {
        if (isHistSliceIdsValid(histSliceIds)) {
            if (filterByCurrSlice(_selectedHistIds).size() >= 2) {
                _selectedHistIds.clear();
                _selectedHistIds.push_back(sliceIdsToHistIds(histSliceIds));
            } else {
                auto histIds = sliceIdsToHistIds(histSliceIds);
                auto itr = std::find(_selectedHistIds.begin(),
                        _selectedHistIds.end(), histIds);
                if (_selectedHistIds.end() != itr) {
                    _selectedHistIds.erase(itr);
                } else {
                    _selectedHistIds.clear();
                    _selectedHistIds.push_back(histIds);
                }
            }
        } else {
            _selectedHistIds.clear();
        }
        emitSelectedHistsChanged();

    } else if (
            (Qt::ControlModifier & event->modifiers()) == Qt::ControlModifier) {
        if (isHistSliceIdsValid(histSliceIds)) {
            auto histIds = sliceIdsToHistIds(histSliceIds);
            auto itr = std::find(_selectedHistIds.begin(),
                    _selectedHistIds.end(), histIds);
            if (_selectedHistIds.end() != itr) {
                _selectedHistIds.erase(itr);
            } else {
                _selectedHistIds.push_back(histIds);
            }
            emitSelectedHistsChanged();
        }
    }
//    // print for debug
//    for (auto ids : _selectedHistSliceIds) {
//        std::cout << "(" << ids[0] << "," << ids[1] << ") ";
//    }
//    std::cout << std::endl;
    // update
    update();
}

void HistVolumePhysicalOpenGLView::mouseMoveEvent(QMouseEvent *event) {
    auto mouseCurr = event->localPos();

    _hoveredHistSliceIds = localPositionToHistSliceId(mouseCurr);

    auto mouseDirPixel = QVector2D(mouseCurr - _mousePrev);
    mouseDirPixel.setY(-mouseDirPixel.y());
//    auto mouseDir =
//            QVector2D(
//                (mouseCurr - _mousePrev) / (shf()));
//    mouseDir.setY(-mouseDir.y());
    if (event->buttons() & Qt::LeftButton) {
        //        _camera->orbit(mouseDir);
        QRectF sliceRect = calcSliceRect();
        QVector2D translate(
                mouseDirPixel.x() / sliceRect.width(),
                mouseDirPixel.y() / sliceRect.height());
        _currTranslate -= translate;
        boundSliceTransform();
        updateHistPainterRects();
    }
//    if (event->buttons() & Qt::RightButton)
//        _camera->track(mouseDir);
//    // reset near and far plane
//    auto vol = _avgVolumes[_currVarId];
//    QVector3D bmin(0.f, 0.f, 0.f);
//    QVector3D bmax(vol->w() * vol->sx(), vol->h() * vol->sy(),
//            vol->d() * vol->sz());
//    _camera->resetNearFar(bmin, bmax);
    update();
    _mousePrev = mouseCurr;
}

void HistVolumePhysicalOpenGLView::wheelEvent(QWheelEvent *event) {
    auto numDegrees = float(event->angleDelta().y()) / 8.f;
    _currZoom = _currZoom * (100.f + numDegrees) / 100.f;
    boundSliceTransform();
    updateHistPainterRects();
//    _camera->zoom(numDegrees);
    update();
}

void HistVolumePhysicalOpenGLView::enterEvent(QEvent *) {
}

void HistVolumePhysicalOpenGLView::leaveEvent(QEvent *) {
    _hoveredHistSliceIds = {{-1, -1}};
    update();
}

QRectF HistVolumePhysicalOpenGLView::calcDefaultSliceRect() const {
    int nHistX = _currSlice->nHistX();
    int nHistY = _currSlice->nHistY();
    int w = sw();
    int h = sh();
    QSizeF defaultSliceSize;
    QPointF defaultSliceLowerLeft(0.f, 0.f);
    QPointF defaultSliceUpperRight(w, h);
    auto borderPixel = _borderPixel / 2.f;
    if (float(w) / nHistX < float(h) / nHistY) {
        defaultSliceSize.setWidth(w - 2.f * borderPixel);
        defaultSliceSize.setHeight(
                defaultSliceSize.width() * nHistY / nHistX);
        defaultSliceLowerLeft.setX(borderPixel);
        defaultSliceLowerLeft.setY(
                0.5f * h - 0.5f * defaultSliceSize.height());
        defaultSliceUpperRight.setX(w - borderPixel);
        defaultSliceUpperRight.setY(
                0.5f * h + 0.5f * defaultSliceSize.height());
    } else {
        defaultSliceSize.setHeight(h - 2.f * borderPixel);
        defaultSliceSize.setWidth(
                defaultSliceSize.height() * nHistX / nHistY);
        defaultSliceLowerLeft.setX(
                0.5f * w - 0.5f * defaultSliceSize.width());
        defaultSliceLowerLeft.setY(borderPixel);
        defaultSliceUpperRight.setX(
                0.5f * w + 0.5f * defaultSliceSize.width());
        defaultSliceUpperRight.setY(h - borderPixel);
    }
    QPointF topLeft =
            QPointF(
                defaultSliceLowerLeft.x(), shf() - defaultSliceUpperRight.y());
    QPointF bottomRight =
            QPointF(
                defaultSliceUpperRight.x(), shf() - defaultSliceLowerLeft.y());
    return QRectF(topLeft, bottomRight);
}

QRectF HistVolumePhysicalOpenGLView::calcSliceRect() const {
    int w = sw();
    int h = sh();
    QRectF defaultSliceRect = calcDefaultSliceRect();
    QSizeF defaultSliceSize = defaultSliceRect.size();
    QPointF defaultSliceLowerLeft(
            defaultSliceRect.left(), h - defaultSliceRect.bottom());
    QPointF defaultSliceUpperRight(
            defaultSliceRect.right(), h - defaultSliceRect.top());

    QVector2D defaultSliceSizeVec(
            defaultSliceSize.width(), defaultSliceSize.height());
    QVector2D translate =
            (QVector2D(0.5f, 0.5f) - _currTranslate) * defaultSliceSizeVec;

    QPointF sliceLowerLeft = defaultSliceLowerLeft;
    sliceLowerLeft = translate.toPointF() + sliceLowerLeft;
    QPointF sliceUpperRight = defaultSliceUpperRight;
    sliceUpperRight = translate.toPointF() + sliceUpperRight;

    QPointF center(0.5f * w, 0.5f * h);
    sliceLowerLeft = _currZoom * (sliceLowerLeft - center) + center;
    sliceUpperRight = _currZoom * (sliceUpperRight - center) + center;

    QPointF topLeft(sliceLowerLeft.x(), h - sliceUpperRight.y());
    QPointF bottomRight(sliceUpperRight.x(), h - sliceLowerLeft.y());

    return QRectF(topLeft, bottomRight);
}

QRectF HistVolumePhysicalOpenGLView::calcHistRect(
        std::array<int, 2> histSliceIds) {
    QRectF sliceRect = calcSliceRect();
    auto nHistX = _currSlice->nHistX();
    auto nHistY = _currSlice->nHistY();
    auto histWidth = (sliceRect.width() - (nHistX - 1) * _histSpacing) / nHistX;
    auto histHeight =
            (sliceRect.height() - (nHistY - 1) * _histSpacing) / nHistY;
    auto histLeft =
            sliceRect.left() + histSliceIds[0] * (histWidth + _histSpacing);
    auto sliceBottom = shf() - sliceRect.bottom();
    auto histBottom =
            sliceBottom + histSliceIds[1] * (histHeight + _histSpacing);
    auto histTop = shf() - histBottom - histHeight;
    return QRectF(histLeft, histTop, histWidth, histHeight);
}

void HistVolumePhysicalOpenGLView::boundSliceTransform() {
    int w = sw();
    int h = sh();
    QRectF defaultSliceRect = calcDefaultSliceRect();

    float defaultHistWidth = defaultSliceRect.width() / _currSlice->nHistX();
    float defaultHistHeight = defaultSliceRect.height() / _currSlice->nHistY();
    float maxZoom = std::min((w - 2.f * _borderPixel) / defaultHistWidth,
                             (h - 2.f * _borderPixel) / defaultHistHeight);
    _currZoom = std::min(maxZoom, std::max(_defaultZoom, _currZoom));

    QSizeF sliceSize = defaultSliceRect.size() * _currZoom;
    float left = (0.5f * w - _borderPixel) / sliceSize.width();
    float right = 1.f - (w - _borderPixel - 0.5f * w) / sliceSize.width();
    float bottom = (0.5f * h - _borderPixel) / sliceSize.height();
    float top = 1.f - (h - _borderPixel - 0.5f * h) / sliceSize.height();
    if (right < left) {
        _currTranslate.setX(0.5f);
    } else {
        _currTranslate.setX(
                std::max(left, std::min(right, _currTranslate.x())));
    }
    if (top < bottom) {
        _currTranslate.setY(0.5f);
    } else {
        _currTranslate.setY(
                std::max(bottom, std::min(top, _currTranslate.y())));
    }
}

void HistVolumePhysicalOpenGLView::updateCurrSlice() {
    if (YZ == _currOrien) {
        _currSlice = _histVolume->yzSlice(_currSliceId);
    } else if (XZ == _currOrien) {
        _currSlice = _histVolume->xzSlice(_currSliceId);
    } else if (XY == _currOrien) {
        _currSlice = _histVolume->xySlice(_currSliceId);
    } else {
        assert(false);
    }
    delayForInit([this]() {
        _histPainters.resize(_currSlice->nHist());
        createHistPainters();
        setHistsToHistPainters();
        setRangesToHistPainters();
        updateHistPainterRects();
    });
}

void HistVolumePhysicalOpenGLView::createHistPainters() {
    _histPainters.resize(_currSlice->nHist());
    for (int iHist = 0; iHist < _currSlice->nHist(); ++iHist) {
        _histPainters[iHist] = std::make_shared<HistFacadePainter>();
    }
}

void HistVolumePhysicalOpenGLView::setHistsToHistPainters() {
    int nHistX = _currSlice->nHistX();
    int nHistY = _currSlice->nHistY();
    for (auto x = 0; x < nHistX; ++x)
    for (auto y = 0; y < nHistY; ++y) {
        _histPainters[x + nHistX * y]->setHist(
                _currSlice->hist(x, y), _currDims);
    }
}

void HistVolumePhysicalOpenGLView::setRangesToHistPainters() {
    for (int iHist = 0; iHist < _currSlice->nHist(); ++iHist) {
        float vMin = std::numeric_limits<float>::max();
        float vMax = std::numeric_limits<float>::lowest();
        auto collapsedHist = _currSlice->hist(iHist)->hist(_currDims);
        for (auto v : collapsedHist->values()) {
            vMin = std::min(vMin, float(v));
            vMax = std::max(vMax, float(v));
        }
        _histPainters[iHist]->setRange(vMin, vMax);
    }
}

void HistVolumePhysicalOpenGLView::updateHistPainterRects() {
    int nHistX = _currSlice->nHistX();
    int nHistY = _currSlice->nHistY();
    for (auto x = 0; x < nHistX; ++x)
    for (auto y = 0; y < nHistY; ++y) {
        QRectF histRect = calcHistRect({{x, y}});
        float left = histRect.left() / swf();
        float bottom = (shf() - histRect.bottom()) / shf();
        float histWidth = histRect.width() / swf();
        float histHeight = histRect.height() / shf();
        _histPainters[x + nHistX * y]->setRect(
                left, bottom, histWidth, histHeight);
    }
}

void HistVolumePhysicalOpenGLView::updateSliceIdScrollBar() {
    int nSlices = getSliceCountInDirection(_histVolume->dimHists(), _currOrien);
    LazyUI::instance().labeledScrollBar(tr("sliceId"), tr("Slice Index"),
            0, nSlices - 1, _currSliceId, this, [this](int value) {
        _currSliceId = value;
        updateCurrSlice();
        update();
    });
}

int HistVolumePhysicalOpenGLView::getSliceCountInDirection(
        Extent dimHists, Orien orien) const {
    if (YZ == orien) {
        return dimHists[0];
    }
    if (XZ == orien) {
        return dimHists[1];
    }
    if (XY == orien) {
        return dimHists[2];
    }
    assert(false);
    return -1;
}

std::array<int, 2> HistVolumePhysicalOpenGLView::localPositionToHistSliceId(
        QPointF localPos) const {
    auto sliceRect = calcSliceRect();
    auto histWidth = sliceRect.width() / _currSlice->nHistX();
    auto histHeight = sliceRect.height() / _currSlice->nHistY();
    int x = std::floor((localPos.x() - sliceRect.left()) / histWidth);
    int y = std::floor(
            ((shf() - localPos.y()) - shf() + sliceRect.bottom()) / histHeight);
    return {{x, y}};
}

bool HistVolumePhysicalOpenGLView::isHistSliceIdsValid(
        std::array<int, 2> histSliceIds) const {
    if (histSliceIds[0] < 0 || histSliceIds[0] >= _currSlice->nHistX()) {
        return false;
    }
    if (histSliceIds[1] < 0 || histSliceIds[1] >= _currSlice->nHistY()) {
        return false;
    }
    return true;
}

QColor HistVolumePhysicalOpenGLView::quarterColor() const {
    QColor color = _spacingColor;
    color.setAlphaF(0.45f * color.alphaF());
    return color;
}

QColor HistVolumePhysicalOpenGLView::fullColor() const {
    QColor color = _spacingColor;
    color.setAlphaF(0.75f * color.alphaF());
    return color;
}

void HistVolumePhysicalOpenGLView::emitSelectedHistsChanged() {
    std::vector<std::shared_ptr<const Hist>> hists;
    hists.reserve(_selectedHistIds.size());
    for (auto histIds : _selectedHistIds) {
        auto histFacade = _histVolume->hist(histIds[0], histIds[1], histIds[2]);
        hists.push_back(histFacade->hist(_currDims));
    }
    emit selectedHistsChanged(hists);
}

std::array<int, 3> HistVolumePhysicalOpenGLView::sliceIdsToHistIds(
        std::array<int, 2> histSliceIds) {
    if (YZ == _currOrien) {
        return {{_currSliceId, histSliceIds[0], histSliceIds[1]}};
    }
    if (XZ == _currOrien) {
        return {{histSliceIds[0], _currSliceId, histSliceIds[2]}};
    }
    if (XY == _currOrien) {
        return {{histSliceIds[0], histSliceIds[1], _currSliceId}};
    }
    assert(false);
    return {{-1, -1, -1}};
}

std::vector<std::array<int, 2>> HistVolumePhysicalOpenGLView::filterByCurrSlice(
        const std::vector<std::array<int, 3>> &histIds) {
    std::vector<std::array<int, 2>> histSliceIds;
    for (auto histId : histIds) {
        if (YZ == _currOrien && _currSliceId == histId[0]) {
            histSliceIds.push_back({{histId[1], histId[2]}});
        } else if (XZ == _currOrien && _currSliceId == histId[1]) {
            histSliceIds.push_back({{histId[0], histId[2]}});
        } else if (XY == _currOrien && _currSliceId == histId[2]) {
            histSliceIds.push_back({{histId[0], histId[1]}});
        }
    }
    return histSliceIds;
}
