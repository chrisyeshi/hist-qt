#include "histvolumeview.h"
#include <QComboBox>
#include <QScrollBar>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLDebugLogger>
#include <QMouseEvent>
#include <histpainter.h>
#include <camera.h>
#include <histview.h>
#include <histsliceview.h>
#include <histsliceorienview.h>

HistVolumeView::HistVolumeView(QWidget *parent)
  : Widget(parent)
{
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout([this]() {
        auto horizontalLayout = new QHBoxLayout();
        // layout combo
        _layoutCombo = new QComboBox(this);
        _layoutCombo->addItem(tr("Slice View"));
        connect(_layoutCombo,
                static_cast<void(QComboBox::*)(int)>(
                    &QComboBox::currentIndexChanged),
                this, [this](int layout) {
            this->setLayout(Layout(layout));
        });
        horizontalLayout->addWidget(_layoutCombo);
        /// TODO: consider combining the volume combo and the dimension combo
        /// into a single combo box that has all the options.
        // histogram volume combo
        _histVolumeCombo = new QComboBox(this);
        connect(_histVolumeCombo,
                static_cast<void(QComboBox::*)(const QString&)>(
                    &QComboBox::currentTextChanged),
                this, &HistVolumeView::selectHistVolume);
        horizontalLayout->addWidget(_histVolumeCombo);
        // histogram dimension combo
        _histDimensionCombo = new QComboBox(this);
        connect(_histDimensionCombo,
                static_cast<void(QComboBox::*)(const QString&)>(
                    &QComboBox::currentTextChanged),
                this, &HistVolumeView::selectHistDimension);
        horizontalLayout->addWidget(_histDimensionCombo);
        return horizontalLayout;
    }());
    auto sliceView = new HistVolumeViewSlice(this);
    _stackedLayout = new QStackedLayout();
    _stackedLayout->insertWidget(SLICE, sliceView);
    _impls.insert(SLICE, sliceView);
    verticalLayout->addLayout(_stackedLayout);
}

void HistVolumeView::update()
{
    currentImpl()->update();
}

void HistVolumeView::setHistConfigs(std::vector<HistConfig> configs)
{
    _histConfigs = configs;
    // populate the histogram volume combo box
    _histVolumeCombo->blockSignals(true);
    _histVolumeCombo->clear();
    for (auto config : _histConfigs) {
        _histVolumeCombo->addItem(QString::fromStdString(config.name()));
    }
    _histVolumeCombo->setCurrentIndex(0);
    _histVolumeCombo->blockSignals(false);
    // select the initial histogram volume
    _histName = _histVolumeCombo->itemText(0);
    // populate the histogram dimensions combo box
    updateHistDimensions(configs[0]);
    // select the initial histogram dimensions
    _histDims = _histDimStrToIndVec[_histDimensionCombo->itemText(0)];
    currentImpl()->setHistDimensions(_histDims);
}

void HistVolumeView::setDataStep(std::shared_ptr<DataStep> dataStep)
{
    _dataStep = dataStep;
    currentImpl()->setHistVolume(_dataStep->volume(_histName.toStdString()));
}

void HistVolumeView::setSelectedHistMask(BoolMask3D selectedHistMask)
{
    _selectedHistMask = selectedHistMask;
    currentImpl()->setSelectedHistMask(_selectedHistMask);
}

void HistVolumeView::setLayout(HistVolumeView::Layout layout)
{
    _stackedLayout->setCurrentIndex(layout);
    /// TODO: update the visible widget.
}

void HistVolumeView::selectHistVolume(const QString &name)
{
    const HistConfig& histConfig = [&name, this]() {
        auto itr = std::find_if(_histConfigs.begin(), _histConfigs.end(),
                [&name](const HistConfig& config) {
            return name.toStdString() == config.name();
        });
        return *itr;
    }();
    updateHistDimensions(histConfig);
    _histName = name;
    currentImpl()->setHistVolume(_dataStep->volume(_histName.toStdString()));
    _histDims = _histDimStrToIndVec[_histDimensionCombo->itemText(0)];
    currentImpl()->setHistDimensions(_histDims);
    currentImpl()->update();
}

void HistVolumeView::updateHistDimensions(const HistConfig &histConfig)
{
    // populate the histogram dimension combo box
    _histDimensionCombo->blockSignals(true);
    _histDimensionCombo->clear();
    _histDimStrToIndVec.clear();
    const std::vector<std::string>& vars = histConfig.vars;
    for (int i = 0; i < int(vars.size()); ++i)
    for (int j = i + 1; j < int(vars.size()); ++j) {
        QString dimStr = QString::fromStdString(vars[i] + "-" + vars[j]);
        _histDimStrToIndVec.insert(dimStr, {i, j});
        _histDimensionCombo->addItem(dimStr);
    }
    for (int i = 0; i < int(vars.size()); ++i) {
        QString dimStr = QString::fromStdString(vars[i]);
        _histDimStrToIndVec.insert(dimStr, {i});
        _histDimensionCombo->addItem(dimStr);
    }
    _histDimensionCombo->setCurrentIndex(0);
    _histDimensionCombo->blockSignals(false);
}

void HistVolumeView::selectHistDimension(const QString &dimStr)
{
    auto indVec = _histDimStrToIndVec.value(dimStr);
    assert(!indVec.empty());
    _histDims = indVec;
    currentImpl()->setHistDimensions(_histDims);
    currentImpl()->update();
}

HistVolumeViewImpl *HistVolumeView::currentImpl() const
{
    return _impls[_stackedLayout->currentIndex()];
}

HistVolumeViewSlice::HistVolumeViewSlice(QWidget *parent)
  : QWidget(parent)
  , _sliceIndexScrollBars(NUM_SLICES)
  , _sliceViews(NUM_SLICES)
  , _xySliceIndex(0), _xzSliceIndex(0), _yzSliceIndex(0)
{
    auto gridLayout = new QGridLayout(this);
    {
        _sliceIndexScrollBars[XY] = new QScrollBar(Qt::Horizontal, this);
        _sliceIndexScrollBars[XY]->setPageStep(1);
        gridLayout->addWidget(_sliceIndexScrollBars[XY], 0, 0);
        connect(_sliceIndexScrollBars[XY], &QScrollBar::valueChanged,
                this, &HistVolumeViewSlice::setXYSliceIndex);
        auto xyView = new HistSliceView(this);
        _sliceViews[XY] = xyView;
        gridLayout->addWidget(xyView, 1, 0);
        connect(xyView, &HistSliceView::histClicked,
                this, &HistVolumeViewSlice::setCurrHistFromXYSlice);

        _sliceIndexScrollBars[XZ] = new QScrollBar(Qt::Horizontal, this);
        _sliceIndexScrollBars[XZ]->setPageStep(1);
        gridLayout->addWidget(_sliceIndexScrollBars[XZ], 0, 1);
        connect(_sliceIndexScrollBars[XZ], &QScrollBar::valueChanged,
                this, &HistVolumeViewSlice::setXZSliceIndex);
        auto xzView = new HistSliceView(this);
        _sliceViews[XZ] = xzView;
        gridLayout->addWidget(xzView, 1, 1);
        connect(xzView, &HistSliceView::histClicked,
                this, &HistVolumeViewSlice::setCurrHistFromXZSlice);

        _sliceIndexScrollBars[YZ] = new QScrollBar(Qt::Horizontal, this);
        _sliceIndexScrollBars[YZ]->setPageStep(1);
        gridLayout->addWidget(_sliceIndexScrollBars[YZ], 2, 0);
        connect(_sliceIndexScrollBars[YZ], &QScrollBar::valueChanged,
                this, &HistVolumeViewSlice::setYZSliceIndex);
        auto yzView = new HistSliceView(this);
        _sliceViews[YZ] = yzView;
        gridLayout->addWidget(yzView, 3, 0);
        connect(yzView, &HistSliceView::histClicked,
                this, &HistVolumeViewSlice::setCurrHistFromYZSlice);

        QComboBox* orienCombo = new QComboBox(this);
        gridLayout->addWidget(orienCombo, 2, 1);
        _histOrienWidget = new QStackedWidget(this);
        gridLayout->addWidget(_histOrienWidget, 3, 1);
        _orienView = new HistSliceOrienView(this);
        _histView = new Hist2DView(this);
        _histOrienWidget->addWidget(_orienView);
        _histOrienWidget->addWidget(_histView);

        for (auto row = 0; row < 4; ++row)
            gridLayout->setRowStretch(row, 1);
        for (auto col = 0; col < 2; ++col)
            gridLayout->setColumnStretch(col, 1);
    }
}

void HistVolumeViewSlice::setHistVolume(std::shared_ptr<const HistFacadeVolume> histVolume) {
    _histVolume = histVolume;
    // update the range of the slice index sliders.
    _sliceIndexScrollBars[XY]->blockSignals(true);
    _sliceIndexScrollBars[XY]->setRange(0, _histVolume->dimHists()[XY] - 1);
    _sliceIndexScrollBars[XY]->blockSignals(false);
    _sliceIndexScrollBars[YZ]->blockSignals(true);
    _sliceIndexScrollBars[YZ]->setRange(0, _histVolume->dimHists()[YZ] - 1);
    _sliceIndexScrollBars[YZ]->blockSignals(false);
    _sliceIndexScrollBars[XZ]->blockSignals(true);
    _sliceIndexScrollBars[XZ]->setRange(0, _histVolume->dimHists()[XZ] - 1);
    _sliceIndexScrollBars[XZ]->blockSignals(false);
}

void HistVolumeViewSlice::setHistDimensions(const std::vector<int> &dims)
{
    _histDims = dims;
}

void HistVolumeViewSlice::setSelectedHistMask(BoolMask3D selectedHistMask)
{
    _selectedHistMask = selectedHistMask;
}

void HistVolumeViewSlice::update()
{
    updateSliceViews();
    _sliceViews[XY]->update();
    _sliceViews[YZ]->update();
    _sliceViews[XZ]->update();
    QWidget::update();
}

void HistVolumeViewSlice::updateSliceViews()
{
    _orienView->setHistDims(_histVolume->dimHists());
    _sliceViews[XY]->setHistDimensions(_histDims);
    _sliceViews[YZ]->setHistDimensions(_histDims);
    _sliceViews[XZ]->setHistDimensions(_histDims);
    _orienView->highlightXYSlice(_xySliceIndex);
    _sliceViews[XY]->setHistRect(_histVolume->xySlice(_xySliceIndex));
//    _sliceViews[XY]->setSelectedHistMask(
//            _selectedHistMask.xySlice(_xySliceIndex));
    _orienView->highlightXZSlice(_xzSliceIndex);
    _sliceViews[XZ]->setHistRect(_histVolume->xzSlice(_xzSliceIndex));
//    _sliceViews[XZ]->setSelectedHistMask(
//            _selectedHistMask.xzSlice(_xzSliceIndex));
    _orienView->highlightYZSlice(_yzSliceIndex);
    _sliceViews[YZ]->setHistRect(_histVolume->yzSlice(_yzSliceIndex));
//    _sliceViews[YZ]->setSelectedHistMask(
//            _selectedHistMask.yzSlice(_yzSliceIndex));
}

/// TODO: use an array to store the slice indices.
void HistVolumeViewSlice::setXYSliceIndex(int index)
{
    if (index == _xySliceIndex) return;
    _xySliceIndex = index;
    update();
}

void HistVolumeViewSlice::setXZSliceIndex(int index)
{
    if (index == _xzSliceIndex) return;
    _xzSliceIndex = index;
    update();
}

void HistVolumeViewSlice::setYZSliceIndex(int index)
{
    if (index == _yzSliceIndex) return;
    _yzSliceIndex = index;
    update();
}

void HistVolumeViewSlice::setCurrHistFromXYSlice(
        std::array<int, 2> rectIds, std::vector<int> dims)
{
    setCurrHist({{ rectIds[0], rectIds[1], _xySliceIndex }}, dims);
}

void HistVolumeViewSlice::setCurrHistFromXZSlice(std::array<int, 2> rectIds, std::vector<int> dims)
{
    setCurrHist({{ rectIds[0], _xzSliceIndex, rectIds[1] }}, dims);
}

void HistVolumeViewSlice::setCurrHistFromYZSlice(std::array<int, 2> rectIds, std::vector<int> dims)
{
    setCurrHist({{ _yzSliceIndex, rectIds[0], rectIds[1] }}, dims);
}

void HistVolumeViewSlice::setCurrHist(
        std::array<int, 3> histIds, std::vector<int> dims)
{
    std::shared_ptr<const HistFacade> hist =
            _histVolume->hist(histIds[0], histIds[1], histIds[2]);
    if (hist == _currHist) {
        _currHist = nullptr;
        /// TODO: _histView->setHist(nullptr);
        _histOrienWidget->setCurrentWidget(_orienView);
        return;
    }
    _currHist = hist;
    _histView->setHist(_currHist->hist(dims));
//    HistCollapser collapser(hist);
//    _histView->setHist(collapser.collapseTo(dims));
    _histView->update();
    _histOrienWidget->setCurrentWidget(_histView);
}
