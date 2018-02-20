#include "histvolumephysicalview.h"
#include <QBoxLayout>
#include <QLabel>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QShortcut>
#include <QTimer>
#include <histview.h>
#include <lazyui.h>
#include <histfacadepainter.h>
#include <histcharter.h>
#include <painter.h>

namespace {

const QColor bgColor = QColor(0.8f * 255, 0.8f * 255, 0.8f * 255, 0.8f * 255);

float clamp(float val, float min, float max) {
    return std::min(max, std::max(min, val));
}

template <typename T>
void extendSet(std::vector<T>& recvSet, const std::vector<T>& mergingSet) {
    for (const auto& element : mergingSet) {
        auto itr = std::find(recvSet.begin(), recvSet.end(), element);
        if (recvSet.end() == itr) {
            recvSet.push_back(element);
        }
    }
}

float calcAverage(std::shared_ptr<const Hist1D> hist) {
    double sum = 0.f;
    for (int iBin = 0; iBin < hist->nBins(); ++iBin) {
        double deltaBinValue =
                (hist->dimMax(0) - hist->dimMin(0)) / hist->nBins();
        double beginBinValue = 0.5 * deltaBinValue;
        double binValue = beginBinValue + iBin * deltaBinValue;
        double frequency = hist->binFreq(iBin);
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
        varsToDims.insert(str, {i, j});
    }
    return varsToDims;
}

QMap<std::vector<int>, QString> getDimsToVars(const HistConfig& config) {
    QMap<std::vector<int>, QString> dimsToVars;
    for (int i = 0; i < int(config.vars.size()); ++i) {
        QString str = QString::fromStdString(config.vars[i]);
        dimsToVars.insert({i}, str);
    }
    for (int i = 0; i < int(config.vars.size()); ++i)
    for (int j = i + 1; j < int(config.vars.size()); ++j) {
        QString str =
                QString::fromStdString(config.vars[i] + "-" + config.vars[j]);
        dimsToVars.insert({i, j}, str);
    }
    return dimsToVars;
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

template <typename T>
std::array<float, 2> calcFreqRange(
        const T& hists, const std::vector<int> dims) {
    float vMin = std::numeric_limits<float>::max();
    float vMax = std::numeric_limits<float>::lowest();
    for (int iHist = 0; iHist < hists->nHist(); ++iHist) {
        auto collapsedHist = hists->hist(iHist)->hist(dims);
        for (auto iBin = 0; iBin < collapsedHist->nBins(); ++iBin) {
            if (collapsedHist->binPercent(iBin) < 0.f)
                continue;
            vMin = std::min(vMin, collapsedHist->binPercent(iBin));
            vMax = std::max(vMax, collapsedHist->binPercent(iBin));
        }
    }
    return {vMin, vMax};
}

std::array<float, 2> calcFreqRange(const std::shared_ptr<const Hist>& hist) {
    float vMin = std::numeric_limits<float>::max();
    float vMax = std::numeric_limits<float>::lowest();
    for (auto iBin = 0; iBin < hist->nBins(); ++iBin) {
        if (hist->binPercent(iBin) < 0.f)
            continue;
        vMin = std::min(vMin, hist->binPercent(iBin));
        vMax = std::max(vMax, hist->binPercent(iBin));
    }
    return {vMin, vMax};
}

QVector2D calcDeltaVecPixel(const QList<QTouchEvent::TouchPoint>& touchPoints) {
    QVector2D prev(0.f, 0.f), curr(0.f, 0.f);
    int nPts = touchPoints.size();
    for (auto pt : touchPoints) {
        prev += QVector2D(pt.lastPos());
        curr += QVector2D(pt.pos());
    }
    return curr / nPts - prev / nPts;
}

} // unnamed namespace

/**
 * @brief HistVolumePhysicalView::HistVolumePhysicalView
 * @param parent
 */
HistVolumePhysicalView::HistVolumePhysicalView(QWidget *parent)
      : HistVolumeView(parent)
      , _histVolumeView(new HistVolumePhysicalOpenGLView(this)) {
    qRegisterMetaType<std::string>();
    auto hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(5);
    hLayout->addWidget(_histVolumeView, 3);
    connect(_histVolumeView,
            &HistVolumePhysicalOpenGLView::currHistConfigDimsChanged,
            this,
            [this](std::string name, std::vector<int> displayDims) {
        _histDims = displayDims;
        emit currHistConfigDimsChanged(name, displayDims);
    });
    connect(_histVolumeView,
            SIGNAL(
                selectedHistsChanged(std::vector<std::shared_ptr<const Hist>>)),
            this,
            SIGNAL(
                selectedHistsChanged(
                    std::vector<std::shared_ptr<const Hist>>)));
    connect(_histVolumeView,
            &HistVolumePhysicalOpenGLView::selectedHistIdsChanged,
            this,
            [this](std::vector<int> flatIds, std::vector<int> displayDims) {
        emit selectedHistIdsChanged(currHistName(), flatIds, displayDims);
    });
    connect(_histVolumeView,
            &HistVolumePhysicalOpenGLView::customVarRangesChanged,
            this,
            [this](const std::vector<std::array<double, 2>>& varRanges) {
        HistRangesMap varRangesMap;
        for (int i = 0; i < _histDims.size(); ++i) {
            varRangesMap[_histDims[i]] = varRanges[i];
        }
        emit customVarRangesChanged(varRangesMap);
    });
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
}

void HistVolumePhysicalView::setDataStep(std::shared_ptr<DataStep> dataStep) {
    _dataStep = dataStep;
}

void HistVolumePhysicalView::setCustomHistRanges(
        const HistVolumeView::HistRangesMap &histRangesMap) {
    _histVolumeView->setCustomHistRanges(histRangesMap);
    _histVolumeView->update();
}

void HistVolumePhysicalView::setCurrHistVolume(const QString &histVolumeName) {
    QStringList names = getHistConfigNames(_histConfigs);
    assert(names.contains(histVolumeName));
    _currHistConfigId = names.indexOf(histVolumeName);
}

void HistVolumePhysicalView::reset(std::vector<int> displayDims) {
    _histVolumeView->reset(displayDims);
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
    // keyboard shortcuts
    auto ctrlUp = new QShortcut(QKeySequence(tr("ctrl+up")), this);
    connect(ctrlUp, &QShortcut::activated, this, [this]() {
        int nSlices =
                getSliceCountInDirection(_histVolume->dimHists(), _currOrien);
        _currSliceId = clamp(_currSliceId + 1, 0, nSlices - 1);
        qInfo() << "sliceIdKeyboard" << _currSliceId;
        updateCurrSlice();
        LazyUI::instance().labeledScrollBar(tr("sliceId"), _currSliceId);
        render();
        update();
    });
    auto ctrlDown = new QShortcut(QKeySequence(tr("ctrl+down")), this);
    connect(ctrlDown, &QShortcut::activated, this, [this]() {
        int nSlices =
                getSliceCountInDirection(_histVolume->dimHists(), _currOrien);
        _currSliceId = clamp(_currSliceId - 1, 0, nSlices - 1);
        qInfo() << "sliceIdKeyboard" << _currSliceId;
        updateCurrSlice();
        LazyUI::instance().labeledScrollBar(tr("sliceId"), _currSliceId);
        render();
        update();
    });

    qRegisterMetaType<std::vector<std::shared_ptr<const Hist>>>();
    setMouseTracking(true);
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    QTimer::singleShot(0, this, [=]() {
        LazyUI::instance().sectionHeader(
                "histSliceOrientation", "Histogram Slice Orientation");
        LazyUI::instance().labeledCombo(
                tr("sliceDirections"), tr("Slicing Direction"),
                {tr("XY"), tr("XZ"), tr("YZ")}, this,
                [this](const QString& text) {
            qInfo() << "sliceDirectionsCombo" << text;
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
            render();
            update();
        });
        LazyUI::instance().labeledScrollBar(
                tr("sliceId"), tr("Slice Index"), FluidLayout::Item::FullLine);
        LazyUI::instance().sectionHeader("freqHeader", "Frequency Range");
        LazyUI::instance().labeledCombo(
                "freqRangeMethod", "Per",
                {"Histogram", "Histogram Volume", "Histogram Slice", "Custom"},
                FluidLayout::Item::Large, this, [=](const QString& text) {
            qInfo() << "freqRangeMethodCombo" << text;
            if (tr("Histogram") == text) {
                _currFreqNormPer = NormPer_Histogram;
            } else if (tr("Histogram Slice") == text) {
                _currFreqNormPer = NormPer_HistSlice;
            } else if (tr("Histogram Volume") == text) {
                _currFreqNormPer = NormPer_HistVolume;
            } else if (tr("Custom") == text) {
                _currFreqNormPer = NormPer_Custom;
            } else {
                assert(false);
            }
            _currFreqRange = calcFreqRange();
            setFreqRangesToHistPainters(_currFreqRange);
            LazyUI::instance().labeledLineEdit(
                    "freqRangeMin", QString::number(_currFreqRange[0]));
            LazyUI::instance().labeledLineEdit(
                    "freqRangeMax", QString::number(_currFreqRange[1]));
            render();
            update();
        });
        LazyUI::instance().labeledLineEdit("freqRangeMin", "Minimum",
                "nan", FluidLayout::Item::Medium, this,
                [=](const QString& text) {
            qInfo() << "freqRangeMinLineEdit" << text;
            _currFreqNormPer = NormPer_Custom;
            LazyUI::instance().labeledCombo("freqRangeMethod", "Custom");
            _currFreqRange[0] = text.toDouble();
            setFreqRangesToHistPainters(_currFreqRange);
            render();
            update();
        });
        LazyUI::instance().labeledLineEdit("freqRangeMax", "Maximum",
                "nan", FluidLayout::Item::Medium, this,
                [=](const QString& text) {
            qInfo() << "freqRangeMaxLineEdit" << text;
            _currFreqNormPer = NormPer_Custom;
            LazyUI::instance().labeledCombo("freqRangeMethod", "Custom");
            _currFreqRange[1] = text.toDouble();
            setFreqRangesToHistPainters(_currFreqRange);
            render();
            update();
        });
        LazyUI::instance().sectionHeader("histHeader", "Histogram Ranges");
        LazyUI::instance().labeledCombo(
                "histRangeMethod", "Per",
                {"Histogram", "Histogram Volume", "Histogram Slice", "Custom"},
                FluidLayout::Item::Large, this, [=](const QString& text) {
            qInfo() << "histRangeMethodCombo" << text;
            if (tr("Histogram") == text) {
                _currHistNormPer = NormPer_Histogram;
            } else if (tr("Histogram Slice") == text) {
                _currHistNormPer = NormPer_HistSlice;
            } else if (tr("Histogram Volume") == text) {
                _currHistNormPer = NormPer_HistVolume;
            } else if (tr("Custom") == text) {
                _currHistNormPer = NormPer_Custom;
            } else {
                assert(false);
            }
            _currHistRanges = calcHistRanges();
            setHistRangesToHistPainters(_currHistRanges);
            LazyUI::instance().labeledLineEdit(
                    "histRange1Min", QString::number(_currHistRanges[0][0]));
            LazyUI::instance().labeledLineEdit(
                    "histRange1Max", QString::number(_currHistRanges[0][1]));
            LazyUI::instance().labeledLineEdit(
                    "histRange2Min", QString::number(_currHistRanges[1][0]));
            LazyUI::instance().labeledLineEdit(
                    "histRange2Max", QString::number(_currHistRanges[1][1]));
            render();
            update();
        });
        LazyUI::instance().labeledLineEdit(
                "histRange1Min", "X Minimum",
                tr("nan"), FluidLayout::Item::Medium, this,
                [=](const QString& text) {
            qInfo() << "histRange1MinLineEdit" << text;
            _currHistNormPer = NormPer_Custom;
            LazyUI::instance().labeledCombo("histRangeMethod", "Custom");
            _currHistRanges[0][0] = text.toDouble();
            setHistRangesToHistPainters(_currHistRanges);
            emit customVarRangesChanged(_currHistRanges);
            render();
            update();
        });
        LazyUI::instance().labeledLineEdit(
                "histRange1Max", "X Maximum",
                tr("nan"), FluidLayout::Item::Medium, this,
                [=](const QString& text) {
            qInfo() << "histRange1MaxLineEdit" << text;
            _currHistNormPer = NormPer_Custom;
            LazyUI::instance().labeledCombo("histRangeMethod", "Custom");
            _currHistRanges[0][1] = text.toDouble();
            setHistRangesToHistPainters(_currHistRanges);
            emit customVarRangesChanged(_currHistRanges);
            render();
            update();
        });
        LazyUI::instance().labeledLineEdit(
                "histRange2Min", "Y Minimum",
                tr("nan"), FluidLayout::Item::Medium, this,
                [=](const QString& text) {
            qInfo() << "histRange2MinLineEdit" << text;
            _currHistNormPer = NormPer_Custom;
            LazyUI::instance().labeledCombo("histRangeMethod", "Custom");
            _currHistRanges[1][0] = text.toDouble();
            setHistRangesToHistPainters(_currHistRanges);
            emit customVarRangesChanged(_currHistRanges);
            render();
            update();
        });
        LazyUI::instance().labeledLineEdit(
                "histRange2Max", "Y Maximum",
                tr("nan"), FluidLayout::Item::Medium, this,
                [=](const QString& text) {
            qInfo() << "histRange2MaxLineEdit" << text;
            _currHistNormPer = NormPer_Custom;
            LazyUI::instance().labeledCombo("histRangeMethod", "Custom");
            _currHistRanges[1][1] = text.toDouble();
            setHistRangesToHistPainters(_currHistRanges);
            emit customVarRangesChanged(_currHistRanges);
            render();
            update();
        });
    });

    delayForInit([this]() {
        glClearColor(
                bgColor.redF(), bgColor.greenF(), bgColor.blueF(),
                bgColor.alphaF());
        _histSliceFbo = createWidgetSizeFbo();
//        _volren =
//                yy::volren::VolRenFactory::create(
//                    yy::volren::Method_Raycast_GL);
//        _volren->initializeGL();
    });
}

void HistVolumePhysicalOpenGLView::setHistVolume(
        HistConfig histConfig,
        std::shared_ptr<HistFacadeVolume> histVolume) {
    if (_histConfig != histConfig) {
        _histConfig = histConfig;
        _currDims = _defaultDims;
        _currSliceId = _defaultSliceId;
    }
    _histVolume = histVolume;
    LazyUI::instance().labeledCombo(
            tr("histVar"), tr("Histogram Variables"),
            getHistDimVars(histConfig), getDimsToVars(_histConfig)[_currDims],
            FluidLayout::Item::Large, this, [=](const QString& text) {
        qInfo() << "histVarCombo" << text;
        auto varsToDims = getVarsToDims(histConfig);
        std::vector<int> dims = varsToDims[text];
        assert(!dims.empty());
        _currDims = dims;
        emitCurrHistConfigDims();
        emitSelectedHistsChanged();
        updateSliceIdScrollBar();
        updateCurrSlice();
        render();
        update();
    });
    updateSliceIdScrollBar();
    emitCurrHistConfigDims();
    emitSelectedHistsChanged();
    delayForInit([this]() {
        updateCurrSlice();
        render();
    });
}

void HistVolumePhysicalOpenGLView::setCustomHistRanges(
        const HistRangesMap &histRangesMap) {
    _currHistNormPer = NormPer_Custom;
    for (auto dimRange : histRangesMap) {
        _currHistRanges[dimRange.first][0] = dimRange.second[0];
        _currHistRanges[dimRange.first][1] = dimRange.second[1];
    }
    LazyUI::instance().labeledCombo("histRangeMethod", "Custom");
    LazyUI::instance().labeledLineEdit(
            "histRange1Min", QString::number(_currHistRanges[0][0]));
    LazyUI::instance().labeledLineEdit(
            "histRange1Max", QString::number(_currHistRanges[0][1]));
    LazyUI::instance().labeledLineEdit(
            "histRange2Min", QString::number(_currHistRanges[1][0]));
    LazyUI::instance().labeledLineEdit(
            "histRange2Max", QString::number(_currHistRanges[1][1]));
    setHistRangesToHistPainters(_currHistRanges);
    render();
}

void HistVolumePhysicalOpenGLView::reset(std::vector<int> displayDims) {
    _currDims = displayDims;
    _currOrien = _defaultOrien;
    _currSliceId = _defaultSliceId;
    _currFreqNormPer = _defaultFreqNormPer;
    _currFreqRange = calcFreqRange();
    _currHistNormPer = _defaultHistNormPer;
    _currHistRanges = calcHistRanges();
    _currTranslate = _defaultTranslate;
    _currZoom = _defaultZoom;
    // update internal states
    updateSliceIdScrollBar();
    _selectedHistIds.clear();
    delayForInit([this]() {
        updateCurrSlice();
        render();
        update();
    });
    // update UI
    auto dimsToVars = getDimsToVars(_histConfig);
    LazyUI::instance().labeledCombo(tr("histVar"), dimsToVars[_currDims]);
    if (YZ == _currOrien) {
        LazyUI::instance().labeledCombo(tr("sliceDirections"), tr("YZ"));
    } else if (XZ == _currOrien) {
        LazyUI::instance().labeledCombo(tr("sliceDirections"), tr("XZ"));
    } else if (XY == _currOrien) {
        LazyUI::instance().labeledCombo(tr("sliceDirections"), tr("XY"));
    }
    LazyUI::instance().labeledCombo("freqRangeMethod", "Histogram");
    LazyUI::instance().labeledLineEdit(
            "freqRangeMin", QString::number(_currFreqRange[0]));
    LazyUI::instance().labeledLineEdit(
            "freqRangeMax", QString::number(_currFreqRange[1]));
    LazyUI::instance().labeledCombo("histRangeMethod", "Histogram");
    LazyUI::instance().labeledLineEdit(
            "histRange1Min", QString::number(_currHistRanges[0][0]));
    LazyUI::instance().labeledLineEdit(
            "histRange1Max", QString::number(_currHistRanges[0][1]));
    LazyUI::instance().labeledLineEdit(
            "histRange2Min", QString::number(_currHistRanges[1][0]));
    LazyUI::instance().labeledLineEdit(
            "histRange2Max", QString::number(_currHistRanges[1][1]));
}

void HistVolumePhysicalOpenGLView::resizeGL(int w, int h) {
    if (!_histVolume) {
        return;
    }
    boundSliceTransform();
    updateHistPainterRects();
    static QTimer* timer = [this]() {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this]() {
            delayForInit([this]() {
                _histSliceFbo = createWidgetSizeFbo();
            });
            render();
            update();
        });
        return timer;
    }();
    timer->start(10);
//    _camera->setAspectRatio(float(w) / float(h));
//    _volren->resize(w, h);
}

void HistVolumePhysicalOpenGLView::paintGL() {
    if (!_histVolume) {
        return;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QOpenGLFramebufferObject::blitFramebuffer(nullptr, _histSliceFbo.get());
    // hovered histogram
    if (isHistSliceIdsValid(_hoveredHistSliceIds)) {
        Painter painter(this);
        painter.setPen(
                QPen(quarterColor(), 6.f * _histSpacing / devicePixelRatioF()));
        painter.drawRect(calcHistRect(_hoveredHistSliceIds));
    }
    // selected histograms
    auto selectedHistSliceIds = filterByCurrSlice(_selectedHistIds);
    if (!selectedHistSliceIds.empty()) {
        Painter painter(this);
        painter.setPen(
                QPen(fullColor(), 6.f * _histSpacing / devicePixelRatioF()));
        for (auto histSliceIds : selectedHistSliceIds) {
            painter.drawRect(calcHistRect(histSliceIds));
        }
    }
    // lasso
    if (_lasso.ongoing()) {
        Painter painter(this);
        painter.fillRect(
                QRectF(_lasso.initLocalPos, _lasso.currLocalPos),
                quarterColor());
    }
//    _volren->setMatVP(_camera->matView(), _camera->matProj());
//    _volren->render();
//    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    _volren->output()->draw();
}

bool HistVolumePhysicalOpenGLView::event(QEvent *event) {
    if (QEvent::Gesture == event->type()) {
        auto gestureEvent = static_cast<QGestureEvent*>(event);
        if (QGesture* gesture = gestureEvent->gesture(Qt::PinchGesture)) {
            gestureEvent->accept();
            QPinchGesture* pinch = static_cast<QPinchGesture*>(gesture);
            zoomEvent(pinch->scaleFactor(), QVector2D(pinch->centerPoint()));
            render();
            update();
            return true;
        }
    }
    return OpenGLWidget::event(event);
}

void HistVolumePhysicalOpenGLView::lassoBeginEvent(const QPointF& localPos) {
    _lasso.state = Lasso::Initiate;
    _lasso.initLocalPos = localPos;
    _lasso.currLocalPos = localPos;
    update();
}

void HistVolumePhysicalOpenGLView::lassoEndEvent(const QPointF &localPos) {
    _lasso.state = Lasso::Idle;
    _lasso.endLocalPos = localPos;
    auto initHistSliceIds = localPositionToHistSliceId(_lasso.initLocalPos);
    auto endHistSliceIds = localPositionToHistSliceId(_lasso.endLocalPos);
    int xDelta = endHistSliceIds[0] - initHistSliceIds[0];
    int yDelta = endHistSliceIds[1] - initHistSliceIds[1];
    int xInc = xDelta ? xDelta / std::abs(xDelta) : 1;
    int yInc = yDelta ? yDelta / std::abs(yDelta) : 1;
    std::vector<std::array<int, 3>> lassoHistIds;
    for (int x = initHistSliceIds[0]; x != endHistSliceIds[0] + xInc; x += xInc)
    for (int y = initHistSliceIds[1]; y != endHistSliceIds[1] + yInc;
            y += yInc) {
        std::array<int, 2> histSliceIds = {x, y};
        if (isHistSliceIdsValid(histSliceIds))
            lassoHistIds.push_back(sliceIdsToHistIds(histSliceIds));
    }
    extendSet(_selectedHistIds, lassoHistIds);
    emitSelectedHistsChanged();
    update();
}

void HistVolumePhysicalOpenGLView::lassoDragEvent(const QPointF &localPos) {
    _lasso.state = Lasso::Dragging;
    _lasso.currLocalPos = localPos;
    update();
}

void HistVolumePhysicalOpenGLView::mousePressEvent(QMouseEvent *event) {
    _mousePress = event->localPos();
    _mousePrev = event->localPos();
    bool isLeftButton = (Qt::LeftButton & event->buttons()) == Qt::LeftButton;
    bool isControlPressed =
            (Qt::ControlModifier & event->modifiers()) == Qt::ControlModifier;
    if (isLeftButton && isControlPressed) {
        lassoBeginEvent(event->localPos());
        return;
    }
}

void HistVolumePhysicalOpenGLView::mouseReleaseEvent(QMouseEvent *event) {
    auto mouseCurr = event->localPos();
    QVector2D mouseDelta(mouseCurr - _mousePress);
    if (mouseDelta.length() < _clickDelta) {
        mouseClickEvent(event);
        return;
    }
    bool isLeftButton = (Qt::LeftButton & event->buttons()) == Qt::LeftButton;
    bool isControlPressed =
            (Qt::ControlModifier & event->modifiers()) == Qt::ControlModifier;
    if (isLeftButton && isControlPressed) {
        lassoEndEvent(event->localPos());
        return;
    }
}

void HistVolumePhysicalOpenGLView::mouseClickEvent(QMouseEvent *event) {
    auto histSliceIds = localPositionToHistSliceId(event->localPos());
    bool isControlPressed =
            (Qt::ControlModifier & event->modifiers()) == Qt::ControlModifier;
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

    } else if (isControlPressed) {
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
    // update
    update();
}

void HistVolumePhysicalOpenGLView::mouseMoveEvent(QMouseEvent *event) {
    if (!_histVolume) {
        return;
    }
    auto mouseCurr = event->localPos();
    // hovered histogram
    _hoveredHistSliceIds = localPositionToHistSliceId(mouseCurr);
    // lasso
    bool isLeftButton = (Qt::LeftButton & event->buttons()) == Qt::LeftButton;
    bool isControlPressed =
            (Qt::ControlModifier & event->modifiers()) == Qt::ControlModifier;
    if (isLeftButton && isControlPressed) {
        lassoDragEvent(mouseCurr);
        return;
    }
    if (_lasso.ongoing() && (!isLeftButton || !isControlPressed)) {
        lassoEndEvent(mouseCurr);
        return;
    }
    // dragging the histogram volume slice
    auto mouseDirPixel = QVector2D(mouseCurr - _mousePrev);
    mouseDirPixel.setY(-mouseDirPixel.y());
//    auto mouseDir =
//            QVector2D(
//                (mouseCurr - _mousePrev) / (shf()));
//    mouseDir.setY(-mouseDir.y());
    if (event->buttons() & Qt::LeftButton) {
        //        _camera->orbit(mouseDir);
        translateEvent(QVector2D(mouseCurr - _mousePrev));
        render();
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
    if (Qt::MouseEventNotSynthesized != event->source()) {
        translateEvent(QVector2D(event->pixelDelta()));
        render();
        update();
        return;
    }
    auto numDegrees = float(event->angleDelta().y()) / 8.f;
    zoomEvent((100.f + numDegrees) / 100.f, QVector2D(event->posF()));
    render();
    update();
}

void HistVolumePhysicalOpenGLView::enterEvent(QEvent *) {
}

void HistVolumePhysicalOpenGLView::leaveEvent(QEvent *) {
    _hoveredHistSliceIds = {{-1, -1}};
    update();
}

// call render() to render the histogram volume slice, call update() to update
// the elements on top of the slice.
void HistVolumePhysicalOpenGLView::render() {
    delayForInit([this]() {
        _histSliceFbo->bind();
        QOpenGLPaintDevice device(
                _histSliceFbo->width(), _histSliceFbo->height());
        device.setDevicePixelRatio(devicePixelRatio());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, _histSliceFbo->width(), _histSliceFbo->height());
        QRectF rect = calcSliceRect();
        float dx = rect.width() / _currSlice->nHistX();
        float dy = rect.height() / _currSlice->nHistY();
        if (_currDims.size() != 1 || dx > _sizeThresholdToRenderSolidColor) {
            // if there is space for plotting the histograms
            {
                Painter bgPainter(&device);
                bgPainter.fillRect(rect, Qt::white);
                bgPainter.setPen(QPen(bgColor, 0.75f));
                // vertical grid lines separating the histograms
                for (int iHistX = 0; iHistX <= _currSlice->nHistX(); ++iHistX) {
                    float x = iHistX * dx + rect.left();
                    bgPainter.drawLine(QLineF(x, rect.top(), x, rect.bottom()));
                }
                // horizontal grid lines separating the histograms
                for (int iHistY = 0; iHistY <= _currSlice->nHistY(); ++iHistY) {
                    float y = iHistY * dy + rect.top();
                    bgPainter.drawLine(QLineF(rect.left(), y, rect.right(), y));
                }
            }
//            if (_histPainters.size() > 138) {
//                _histPainters[137]->paint(&device);
//                _histPainters[138]->paint(&device);
//            }
            for (auto painter : _histPainters) {
                painter->paint(&device);
            }
        } else {
            // only draw a solid color based on the average values.
            assert(1 == _currDims.size());
            Painter painter(&device);
            const HistFacadeVolume::Stats& stats = _histVolume->stats();
            for (int iHistY = 0; iHistY < _currSlice->nHistY(); ++iHistY)
            for (int iHistX = 0; iHistX < _currSlice->nHistX(); ++iHistX) {
                QRectF histRect = calcHistRect({iHistX, iHistY});
                auto hist = _currSlice->hist(iHistX, iHistY)->hist();
                int dim = _currDims[0];
                float average = hist->means()[dim];
                const std::string& var = _histConfig.vars[dim];
                const auto& range = stats.meanRanges.at(var);
                float ratio = (average - range[0]) / (range[1] - range[0]);
                glm::vec3 lower = {0.9f, 0.9f, 1.f};
                glm::vec3 higher = {0.2f, 0.3f, 0.6f};
                glm::vec3 color = ratio * (higher - lower) + lower;
                painter.fillRect(
                        histRect,
                        QColor(
                            color[0] * 255, color[1] * 255, color[2] * 255,
                            255));
            }
            /// TODO: try drawing two triangles for 2d histograms.
        }
        _histSliceFbo->release();
    });
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

QRectF HistVolumePhysicalOpenGLView::calcSliceRect(
        float zoom, QVector2D trans) const {
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
            (QVector2D(0.5f, 0.5f) - trans) * defaultSliceSizeVec;

    QPointF sliceLowerLeft = defaultSliceLowerLeft;
    sliceLowerLeft = translate.toPointF() + sliceLowerLeft;
    QPointF sliceUpperRight = defaultSliceUpperRight;
    sliceUpperRight = translate.toPointF() + sliceUpperRight;

    QPointF center(0.5f * w, 0.5f * h);
    sliceLowerLeft = zoom * (sliceLowerLeft - center) + center;
    sliceUpperRight = zoom * (sliceUpperRight - center) + center;

    QPointF topLeft(sliceLowerLeft.x(), h - sliceUpperRight.y());
    QPointF bottomRight(sliceUpperRight.x(), h - sliceLowerLeft.y());

    return QRectF(topLeft, bottomRight);
}

QRectF HistVolumePhysicalOpenGLView::calcSliceRect() const {
    return calcSliceRect(_currZoom, _currTranslate);
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

float HistVolumePhysicalOpenGLView::calcMaxZoom(
        const QRectF& defaultSliceRect) const {
    float defaultHistWidth = defaultSliceRect.width() / _currSlice->nHistX();
    float defaultHistHeight = defaultSliceRect.height() / _currSlice->nHistY();
    return std::min((sw() - 2.f * _borderPixel) / defaultHistWidth,
            (sh() - 2.f * _borderPixel) / defaultHistHeight);
}

float HistVolumePhysicalOpenGLView::calcMaxZoom() const {
    return calcMaxZoom(calcDefaultSliceRect());
}

void HistVolumePhysicalOpenGLView::boundSliceTransform() {
    int w = sw();
    int h = sh();
    QRectF defaultSliceRect = calcDefaultSliceRect();

    _currZoom = clamp(_currZoom, _defaultZoom, calcMaxZoom(defaultSliceRect));

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
        createHistPainters();
        setHistsToHistPainters();
        setFreqRangesToHistPainters();
        setHistRangesToHistPainters();
        updateHistPainterRects();
    });
}

void HistVolumePhysicalOpenGLView::createHistPainters() {
    _histPainters.resize(_currSlice->nHist());
    for (int iHist = 0; iHist < _currSlice->nHist(); ++iHist) {
        _histPainters[iHist] = std::make_shared<HistFacadeCharter>();
        _histPainters[iHist]->setDrawColormap(false);
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

std::array<float, 2> HistVolumePhysicalOpenGLView::calcFreqRange() const {
    if (NormPer_Histogram == _currFreqNormPer) {
        return {NAN, NAN};
    }
    if (NormPer_HistSlice == _currFreqNormPer) {
        return ::calcFreqRange(_currSlice, _currDims);
    }
    if (NormPer_HistVolume == _currFreqNormPer) {
        return ::calcFreqRange(_histVolume, _currDims);
    }
    if (NormPer_Custom == _currFreqNormPer) {
        return _currFreqRange;
    }
    assert(false);
    return {NAN, NAN};
}

void HistVolumePhysicalOpenGLView::setFreqRangesToHistPainters(
        const std::array<float, 2>& range) {
    for (int iHist = 0; iHist < _currSlice->nHist(); ++iHist) {
        auto hist = _currSlice->hist(iHist)->hist(_currDims);
        auto freqRange = ::calcFreqRange(hist);
        std::array<float, 2> r;
        r[0] = std::isnan(range[0]) ? freqRange[0] : range[0];
        r[1] = std::isnan(range[1]) ? freqRange[1] : range[1];
        _histPainters[iHist]->setFreqRange(r[0], r[1]);
    }
}

void HistVolumePhysicalOpenGLView::setFreqRangesToHistPainters() {
    setFreqRangesToHistPainters(calcFreqRange());
}

std::vector<std::array<double, 2>>
        HistVolumePhysicalOpenGLView::calcHistRanges() const {
    if (NormPer_Histogram == _currHistNormPer) {
        return {{NAN, NAN}, {NAN, NAN}};
    }
    if (NormPer_HistSlice == _currHistNormPer) {
        std::vector<std::array<double, 2>> minmaxs(_currDims.size());
        for (auto& range : minmaxs) {
            range[0] = std::numeric_limits<double>::max();
            range[1] = std::numeric_limits<double>::lowest();
        }
        for (int iHist = 0; iHist < _currSlice->nHist(); ++iHist)
        for (int iDim = 0; iDim < _currDims.size(); ++iDim) {
            auto range = _currSlice->hist(iHist)->dimRange(_currDims[iDim]);
            minmaxs[iDim][0] = std::min(range[0], minmaxs[iDim][0]);
            minmaxs[iDim][1] = std::max(range[1], minmaxs[iDim][1]);
        }
        return minmaxs;
    }
    if (NormPer_HistVolume == _currHistNormPer) {
        std::vector<std::array<double, 2>> minmaxs(_currDims.size());
        for (auto& range : minmaxs) {
            range[0] = std::numeric_limits<double>::max();
            range[1] = std::numeric_limits<double>::lowest();
        }
        for (int iHist = 0; iHist < _histVolume->helper().N_HIST; ++iHist)
        for (int iDim = 0; iDim < _currDims.size(); ++iDim) {
            auto range = _histVolume->hist(iHist)->dimRange(_currDims[iDim]);
            minmaxs[iDim][0] = std::min(range[0], minmaxs[iDim][0]);
            minmaxs[iDim][1] = std::max(range[1], minmaxs[iDim][1]);
        }
        return minmaxs;
    }
    if (NormPer_Custom == _currHistNormPer) {
        return _currHistRanges;
    }
    assert(false);
    return {{NAN, NAN}, {NAN, NAN}};
}

void HistVolumePhysicalOpenGLView::setHistRangesToHistPainters(
        const std::vector<std::array<double, 2> > &ranges) {
    for (int iHist = 0; iHist < _currSlice->nHist(); ++iHist) {
        auto hist = _currSlice->hist(iHist);
        std::vector<std::array<double, 2>> minmaxs(_currDims.size());
        for (int i = 0; i < _currDims.size(); ++i) {
            minmaxs[i][0] = std::isnan(ranges[i][0])
                    ? hist->dimRange(_currDims[i])[0]
                    : ranges[i][0];
            minmaxs[i][1] = std::isnan(ranges[i][1])
                    ? hist->dimRange(_currDims[i])[1]
                    : ranges[i][1];
        }
        _histPainters[iHist]->setRanges(minmaxs);
    }
}

void HistVolumePhysicalOpenGLView::setHistRangesToHistPainters() {
    setHistRangesToHistPainters(calcHistRanges());
}

void HistVolumePhysicalOpenGLView::updateHistPainterRects() {
    int nHistX = _currSlice->nHistX();
    int nHistY = _currSlice->nHistY();
    for (auto x = 0; x < nHistX; ++x)
    for (auto y = 0; y < nHistY; ++y) {
        QRectF histRect = calcHistRect({{x, y}});
        float left = histRect.left() / swf();
        float top = histRect.top() / shf();
        float histWidth = histRect.width() / swf();
        float histHeight = histRect.height() / shf();
        _histPainters[x + nHistX * y]->setSize(swf(), shf());
        _histPainters[x + nHistX * y]->setNormalizedViewport(
                left, top, histWidth, histHeight);
    }
}

void HistVolumePhysicalOpenGLView::updateSliceIdScrollBar() {
    int nSlices = getSliceCountInDirection(_histVolume->dimHists(), _currOrien);
    LazyUI::instance().labeledScrollBar(tr("sliceId"), tr("Slice Index"),
            0, nSlices - 1, _currSliceId, FluidLayout::Item::FullLine, this,
            [this](int value) {
        qInfo() << "sliceIdScrollBar" << value;
        _currSliceId = value;
        updateCurrSlice();
        render();
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
    if (std::dynamic_pointer_cast<const HistNullFacade>(
            _currSlice->hist(histSliceIds))) {
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

QColor HistVolumePhysicalOpenGLView::lassoColor() const {
    static QColor color(200, 125, 255, 150);
    return color;
}

void HistVolumePhysicalOpenGLView::emitCurrHistConfigDims() {
    emit currHistConfigDimsChanged(_histConfig.name(), _currDims);
}

void HistVolumePhysicalOpenGLView::emitSelectedHistsChanged() {
    std::vector<std::shared_ptr<const Hist>> hists;
    hists.reserve(_selectedHistIds.size());
    std::vector<int> flatIds(_selectedHistIds.size());
    for (auto i = 0; i < _selectedHistIds.size(); ++i) {
        auto histIds = _selectedHistIds[i];
        flatIds[i] = _histVolume->dimHists().idstoflat(histIds);
        auto histFacade = _histVolume->hist(flatIds[i]);
        hists.push_back(histFacade->hist(_currDims));
    }
    emit selectedHistsChanged(hists);
    emit selectedHistIdsChanged(flatIds, _currDims);
}

std::array<int, 3> HistVolumePhysicalOpenGLView::sliceIdsToHistIds(
        std::array<int, 2> histSliceIds) {
    if (YZ == _currOrien) {
        return {{_currSliceId, histSliceIds[0], histSliceIds[1]}};
    }
    if (XZ == _currOrien) {
        return {{histSliceIds[0], _currSliceId, histSliceIds[1]}};
    }
    if (XY == _currOrien) {
        return {{histSliceIds[0], histSliceIds[1], _currSliceId}};
    }
    assert(false);
    return {{-1, -1, -1}};
}

void HistVolumePhysicalOpenGLView::translateEvent(const QVector2D &delta) {
    QRectF sliceRect = calcSliceRect();
    QVector2D translate(
            delta.x() / sliceRect.width(),
            -delta.y() / sliceRect.height());
    _currTranslate -= translate;
    boundSliceTransform();
    static QTimer* timer = [this]() {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this]() {
            qInfo() << "translate event:" << _currTranslate << _currZoom;
        });
        return timer;
    }();
    timer->start(10);
    updateHistPainterRects();
}

void HistVolumePhysicalOpenGLView::zoomEvent(
        float scale, const QVector2D &pos) {
    QVector2D cursorPos(pos.x(), height() - pos.y());
    QVector2D widgetCenter(0.5f * width(), 0.5f * height());
    float oldZoom = _currZoom;
    auto oldTranslate = _currTranslate;
    float newZoom = clamp(oldZoom * scale, _defaultZoom, calcMaxZoom());
    // the cursor normalized position should stay the same after zooming
    QRectF oldSliceRect = calcSliceRect(oldZoom, oldTranslate);
    QVector2D oldSliceBotLeft(
            oldSliceRect.left(), height() - oldSliceRect.bottom());
    QVector2D oldCursorNormPos =
            (cursorPos - oldSliceBotLeft)
            / QVector2D(oldSliceRect.width(), oldSliceRect.height());
    QRectF zoomedSliceRect = calcSliceRect(newZoom, oldTranslate);
    QVector2D zoomedSliceSize(
            zoomedSliceRect.width(), zoomedSliceRect.height());
    QVector2D newSliceBotLeft = cursorPos - oldCursorNormPos * zoomedSliceSize;
    QVector2D newTranslate = (widgetCenter - newSliceBotLeft) / zoomedSliceSize;
    // update zoom and translate
    _currZoom = newZoom;
    _currTranslate = QVector2D(newTranslate.x(), newTranslate.y());
    // bound the zoom so that it's not too big or too small
    boundSliceTransform();
    static QTimer* timer = [this]() {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this]() {
            qInfo() << "zoom event:" << _currTranslate << _currZoom;
        });
        return timer;
    }();
    timer->start(10);
    updateHistPainterRects();
}

std::shared_ptr<QOpenGLFramebufferObject>
        HistVolumePhysicalOpenGLView::createWidgetSizeFbo() const {
    QOpenGLFramebufferObjectFormat format;
    format.setSamples(4);
    return std::make_shared<QOpenGLFramebufferObject>(
        this->size() * this->devicePixelRatio(), format);
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
