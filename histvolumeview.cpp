#include "histvolumeview.h"
#include <QComboBox>
#include <QPushButton>
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
#include <histfacadecollectionview.h>
#include <histdimscombo.h>
#include <data/histmerger.h>

HistVolumeView::HistVolumeView(QWidget *parent)
  : Widget(parent)
  , _histFacadeCollectionView(nullptr)
{
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setMargin(5);
    verticalLayout->setSpacing(5);
    verticalLayout->addLayout([this]() {
        auto horizontalLayout = new QHBoxLayout();
        horizontalLayout->setMargin(0);
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
        _histDimsCombo = new HistDimsCombo(this);
        connect(_histDimsCombo, &HistDimsCombo::dimsChanged,
                this, &HistVolumeView::selectHistDims);
        horizontalLayout->addWidget(_histDimsCombo);
        return horizontalLayout;
    }());
    auto sliceView = new HistVolumeViewSlice(this);
    _stackedLayout = new QStackedLayout();
    _stackedLayout->insertWidget(SLICE, sliceView);
    _impls.insert(SLICE, sliceView);
    connect(sliceView, &HistVolumeViewSlice::popHist,
            this, &HistVolumeView::popHist);
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
    // select the initial histogram dimensions
    _histDimsCombo->blockSignals(true);
    _histDimsCombo->setItems(configs[0].vars);
    _histDimsCombo->setCurrentIndex(0);
    _histDimsCombo->blockSignals(false);
    _histDims = _histDimsCombo->currentDims();
    currentImpl()->setHistDimensions(_histDims);
}

void HistVolumeView::setDataStep(std::shared_ptr<DataStep> dataStep)
{
    disconnect(_dataStep.get(), &DataStep::histSelectionChanged,
            this, &HistVolumeView::repaintSliceViews);
    _dataStep = dataStep;
    connect(_dataStep.get(), &DataStep::histSelectionChanged,
            this, &HistVolumeView::repaintSliceViews);
    currentImpl()->setHistVolume(
            _dataStep->smartVolume(_histName.toStdString()));
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
    _histDimsCombo->blockSignals(true);
    _histDimsCombo->setItems(histConfig.vars);
    _histDimsCombo->setCurrentIndex(0);
    _histDimsCombo->blockSignals(false);
    _histName = name;
    _histDims = _histDimsCombo->currentDims();
    currentImpl()->setHistVolume(
            _dataStep->smartVolume(_histName.toStdString()));
    currentImpl()->setHistDimensions(_histDims);
    currentImpl()->update();
}

void HistVolumeView::selectHistDimension(const QString &dimStr)
{
    auto indVec = _histDimStrToIndVec.value(dimStr);
    assert(!indVec.empty());
    _histDims = indVec;
    currentImpl()->setHistDimensions(_histDims);
    currentImpl()->update();
}

void HistVolumeView::selectHistDims(std::vector<int> dims)
{
    _histDimsCombo->blockSignals(true);
    _histDimsCombo->setCurrentDims(dims);
    _histDimsCombo->blockSignals(false);
    _histDims = _histDimsCombo->currentDims();
    currentImpl()->setHistDimensions(_histDims);
    currentImpl()->update();
}

HistVolumeViewImpl *HistVolumeView::currentImpl() const
{
    return _impls[_stackedLayout->currentIndex()];
}

void HistVolumeView::repaintSliceViews()
{
    currentImpl()->repaintSliceViews();
}

void HistVolumeView::popHist(std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    if (!_histFacadeCollectionView) {
        _histFacadeCollectionView =
                new HistFacadeCollectionView(this, Qt::Window);
        _histFacadeCollectionView->resize(200, 100);
        _histFacadeCollectionView->setWindowTitle("Collection");
        _histFacadeCollectionView->setAttribute(Qt::WA_DeleteOnClose);
        connect(_histFacadeCollectionView, &HistFacadeCollectionView::destroyed,
                this, [this]() {
            _histFacadeCollectionView = nullptr;
        });
    }
    if (histFacade)
        _histFacadeCollectionView->appendHist(histFacade, displayDims);
    _histFacadeCollectionView->show();
}

/**
 * @brief HistVolumeViewSlice::HistVolumeViewSlice
 * @param parent
 */
HistVolumeViewSlice::HistVolumeViewSlice(QWidget *parent)
  : QWidget(parent)
  , _sliceIndexScrollBars(NUM_SLICES)
  , _sliceViews(NUM_SLICES)
  , _xySliceIndex(0), _xzSliceIndex(0), _yzSliceIndex(0)
{
    auto gridLayout = new QGridLayout(this);
    gridLayout->setMargin(0);
    gridLayout->setSpacing(4);
    {
        _sliceIndexScrollBars[XY] = new QScrollBar(Qt::Horizontal, this);
        _sliceIndexScrollBars[XY]->setPageStep(1);
        gridLayout->addWidget(_sliceIndexScrollBars[XY], 0, 0);
        connect(_sliceIndexScrollBars[XY], &QScrollBar::valueChanged,
                this, &HistVolumeViewSlice::setXYSliceIndex);
        auto xyView = new HistSliceView(this);
        xyView->setSpacingColor(QColor(100, 100, 255));
        _sliceViews[XY] = xyView;
        gridLayout->addWidget(xyView, 1, 0);
        connect(xyView, &HistSliceView::histClicked,
                this, &HistVolumeViewSlice::setCurrHistFromXYSlice);
        connect(xyView, &HistSliceView::histHovered,
                this, &HistVolumeViewSlice::setHoveredHistFromXYSlice);
        connect(xyView, &HistSliceView::histMultiSelect,
                this, &HistVolumeViewSlice::multiSelectHistFromXYSlice);

        _sliceIndexScrollBars[XZ] = new QScrollBar(Qt::Horizontal, this);
        _sliceIndexScrollBars[XZ]->setPageStep(1);
        gridLayout->addWidget(_sliceIndexScrollBars[XZ], 0, 1);
        connect(_sliceIndexScrollBars[XZ], &QScrollBar::valueChanged,
                this, &HistVolumeViewSlice::setXZSliceIndex);
        auto xzView = new HistSliceView(this);
        xzView->setSpacingColor(QColor(46, 204, 113));
        _sliceViews[XZ] = xzView;
        gridLayout->addWidget(xzView, 1, 1);
        connect(xzView, &HistSliceView::histClicked,
                this, &HistVolumeViewSlice::setCurrHistFromXZSlice);
        connect(xzView, &HistSliceView::histHovered,
                this, &HistVolumeViewSlice::setHoveredHistFromXZSlice);
        connect(xzView, &HistSliceView::histMultiSelect,
                this, &HistVolumeViewSlice::multiSelectHistFromXZSlice);

        _sliceIndexScrollBars[YZ] = new QScrollBar(Qt::Horizontal, this);
        _sliceIndexScrollBars[YZ]->setPageStep(1);
        gridLayout->addWidget(_sliceIndexScrollBars[YZ], 2, 0);
        connect(_sliceIndexScrollBars[YZ], &QScrollBar::valueChanged,
                this, &HistVolumeViewSlice::setYZSliceIndex);
        auto yzView = new HistSliceView(this);
        yzView->setSpacingColor(QColor(255, 100, 100));
        _sliceViews[YZ] = yzView;
        gridLayout->addWidget(yzView, 3, 0);
        connect(yzView, &HistSliceView::histClicked,
                this, &HistVolumeViewSlice::setCurrHistFromYZSlice);
        connect(yzView, &HistSliceView::histHovered,
                this, &HistVolumeViewSlice::setHoveredHistFromYZSlice);
        connect(yzView, &HistSliceView::histMultiSelect,
                this, &HistVolumeViewSlice::multiSelectHistFromYZSlice);

        QHBoxLayout* orienCtrlLayout = new QHBoxLayout(this);
        orienCtrlLayout->setMargin(0);
        orienCtrlLayout->setSpacing(2);
        gridLayout->addLayout(orienCtrlLayout, 2, 1);
        _orienCombo = new QComboBox(this);
        _orienCombo->addItem(tr("Orientation"));
        _orienCombo->addItem(tr("Histogram"));
        orienCtrlLayout->addWidget(_orienCombo, 1);
        connect(_orienCombo, &QComboBox::currentTextChanged,
                this, &HistVolumeViewSlice::setCurrentOrienWidget);
        QPushButton* histPopButton = new QPushButton(this);
        histPopButton->setText(tr("Pop"));
        orienCtrlLayout->addWidget(histPopButton, 0);
        connect(histPopButton, &QPushButton::clicked, this, [this]() {
            emit popHist(_currHist, _histDims);
        });

        _histOrienWidget = new QStackedWidget(this);
        gridLayout->addWidget(_histOrienWidget, 3, 1);
        _orienView = new HistSliceOrienView(this);
        _histView = new HistView(this);
        _histOrienWidget->addWidget(_orienView);
        _histOrienWidget->addWidget(_histView);

        for (auto row = 0; row < 4; ++row)
            gridLayout->setRowStretch(row, 1);
        for (auto col = 0; col < 2; ++col)
            gridLayout->setColumnStretch(col, 1);
    }
}

void HistVolumeViewSlice::setHistVolume(
        std::shared_ptr<const HistFacadeVolume> histVolume) {
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

void HistVolumeViewSlice::setHistDimensions(const std::vector<int> &dims) {
    _histDims = dims;
    setCurrentOrienWidget(tr("Orientation"));
    unsetCurrHist();
}

void HistVolumeViewSlice::update()
{
    updateSliceViews();
    repaintSliceViews();
    QWidget::update();
}

void HistVolumeViewSlice::repaintSliceViews()
{
    _sliceViews[XY]->updateHistPainters();
    _sliceViews[XY]->update();
    _sliceViews[YZ]->updateHistPainters();
    _sliceViews[YZ]->update();
    _sliceViews[XZ]->updateHistPainters();
    _sliceViews[XZ]->update();
}

void HistVolumeViewSlice::updateSliceViews()
{
    _orienView->setHistDims(_histVolume->dimHists());
    _sliceViews[XY]->setHistDimensions(_histDims);
    _sliceViews[YZ]->setHistDimensions(_histDims);
    _sliceViews[XZ]->setHistDimensions(_histDims);
    _orienView->highlightXYSlice(_xySliceIndex);
    _sliceViews[XY]->setHistRect(_histVolume->xySlice(_xySliceIndex));
    _orienView->highlightXZSlice(_xzSliceIndex);
    _sliceViews[XZ]->setHistRect(_histVolume->xzSlice(_xzSliceIndex));
    _orienView->highlightYZSlice(_yzSliceIndex);
    _sliceViews[YZ]->setHistRect(_histVolume->yzSlice(_yzSliceIndex));
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
        std::array<int, 2> rectIds, std::vector<int> dims) {
    setCurrHist({{ rectIds[0], rectIds[1], _xySliceIndex }}, dims);
}

void HistVolumeViewSlice::setCurrHistFromXZSlice(
        std::array<int, 2> rectIds, std::vector<int> dims) {
    setCurrHist({{ rectIds[0], _xzSliceIndex, rectIds[1] }}, dims);
}

void HistVolumeViewSlice::setCurrHistFromYZSlice(
        std::array<int, 2> rectIds, std::vector<int> dims) {
    setCurrHist({{ _yzSliceIndex, rectIds[0], rectIds[1] }}, dims);
}

void HistVolumeViewSlice::setHoveredHistFromYZSlice(
        std::array<int, 2> rectIds, std::vector<int> dims, bool hovered) {
    setHoveredHist({{ _yzSliceIndex, rectIds[0], rectIds[1] }}, dims, hovered);
}

void HistVolumeViewSlice::setHoveredHistFromXZSlice(
        std::array<int, 2> rectIds, std::vector<int> dims, bool hovered) {
    setHoveredHist({{ rectIds[0], _xzSliceIndex, rectIds[1] }}, dims, hovered);
}

void HistVolumeViewSlice::setHoveredHistFromXYSlice(
        std::array<int, 2> rectIds, std::vector<int> dims, bool hovered) {
    setHoveredHist({{ rectIds[0], rectIds[1], _xySliceIndex }}, dims, hovered);
}

void HistVolumeViewSlice::multiSelectHistFromXYSlice(
        std::array<int,2> rectIds, std::vector<int> dims) {
    multiSelectHist({{ rectIds[0], rectIds[1], _xySliceIndex }}, dims);
}

void HistVolumeViewSlice::multiSelectHistFromXZSlice(
        std::array<int,2> rectIds, std::vector<int> dims) {
    multiSelectHist({{ rectIds[0], _xzSliceIndex, rectIds[1] }}, dims);
}

void HistVolumeViewSlice::multiSelectHistFromYZSlice(
        std::array<int,2> rectIds, std::vector<int> dims) {
    multiSelectHist({{ _yzSliceIndex, rectIds[0], rectIds[1] }}, dims);
}

void HistVolumeViewSlice::multiSelectHist(
        std::array<int,3> histIds, std::vector<int> dims) {
    if (0 < _multiHistIds.count(histIds)) {
        _multiHistIds.erase(histIds);
    } else {
        _multiHistIds.insert(histIds);
    }
    updateCurrHist(dims);
    updateSliceViewsMultiHists();
}

void HistVolumeViewSlice::updateCurrHist(std::vector<int> dims)
{
    if (_multiHistIds.empty()) {
        // unset current histogram
        unsetCurrHist();
        return;
    }
    if (1 == _multiHistIds.size()) {
        // singly set the current histogram
        auto histIds = *_multiHistIds.begin();
        _currHist = _histVolume->hist(histIds[0], histIds[1], histIds[2]);
        _histView->setHist(_currHist, dims);
        _histView->update();
        setCurrentOrienWidget(tr("Histogram"));
        return;
    }
    // merge the multi selected histograms to the current histogram
    std::vector<std::shared_ptr<const Hist>> selectedHists;
    selectedHists.reserve(_multiHistIds.size());
    for (auto histIds : _multiHistIds) {
        std::vector<int> ids(histIds.begin(), histIds.end());
        selectedHists.push_back(_histVolume->hist(ids)->hist());
    }
    std::vector<BinCount> binCounts(
            selectedHists[0]->nDim(), BinCount("freedman"));
    std::shared_ptr<Hist> merged = HistMerger(binCounts).merge(selectedHists);
    _currHist = HistFacade::create(merged, merged->vars());
    _histView->setHist(_currHist, dims);
    _histView->update();
}

void HistVolumeViewSlice::updateSliceViewsMultiHists() {
    std::vector<std::array<int,2>> xyHistIds, xzHistIds, yzHistIds;
    for (auto histIds : _multiHistIds) {
        if (_xySliceIndex == histIds[2])
            xyHistIds.push_back({{ histIds[0], histIds[1] }});
        if (_xzSliceIndex == histIds[1])
            xzHistIds.push_back({{ histIds[0], histIds[2] }});
        if (_yzSliceIndex == histIds[0])
            yzHistIds.push_back({{ histIds[1], histIds[2] }});
    }
    _sliceViews[XY]->setMultiHists(xyHistIds);
    _sliceViews[XY]->update();
    _sliceViews[XZ]->setMultiHists(xzHistIds);
    _sliceViews[XZ]->update();
    _sliceViews[YZ]->setMultiHists(yzHistIds);
    _sliceViews[YZ]->update();
}

void HistVolumeViewSlice::unsetCurrHist()
{
    _currHist = nullptr;
    _histView->setHist(nullptr, {});
    setCurrentOrienWidget(tr("Orientation"));
}

void HistVolumeViewSlice::setCurrHist(
        std::array<int, 3> histIds, std::vector<int> dims) {
    if (1 == _multiHistIds.size() && 0 < _multiHistIds.count(histIds)) {
        // deselect the histogram if it was singly selected.
        _multiHistIds.clear();
    } else {
        // select the only histogram
        _multiHistIds.clear();
        _multiHistIds.insert(histIds);
    }
    updateCurrHist(dims);
    updateSliceViewsMultiHists();
}

void HistVolumeViewSlice::setHoveredHist(
        std::array<int, 3> histIds, std::vector<int> /*dims*/, bool hovered)
{
    _orienView->setHoveredHist(histIds, hovered);
    _sliceViews[YZ]->setHoveredHist({{ histIds[1], histIds[2] }},
            histIds[YZ] == _yzSliceIndex ? hovered : false);
    _sliceViews[YZ]->update();
    _sliceViews[XZ]->setHoveredHist({{ histIds[0], histIds[2] }},
            histIds[XZ] == _xzSliceIndex ? hovered : false);
    _sliceViews[XZ]->update();
    _sliceViews[XY]->setHoveredHist({{ histIds[0], histIds[1] }},
            histIds[XY] == _xySliceIndex ? hovered : false);
    _sliceViews[XY]->update();
}



void HistVolumeViewSlice::setCurrentOrienWidget(QString text) {
    if (tr("Histogram") == text) {
        _orienCombo->blockSignals(true);
        _orienCombo->setCurrentText(tr("Histogram"));
        _orienCombo->blockSignals(false);
        _histOrienWidget->setCurrentWidget(_histView);
    } else if (tr("Orientation") == text) {
        _orienCombo->blockSignals(true);
        _orienCombo->setCurrentText(tr("Orientation"));
        _orienCombo->blockSignals(false);
        _histOrienWidget->setCurrentWidget(_orienView);
    }
}
