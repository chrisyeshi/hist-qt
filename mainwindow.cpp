#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <histview.h>
#include <histvolumesliceview.h>
#include "histvolumephysicalview.h"
#include <histcompareview.h>
#include <data/histmerger.h>
#include <queryview.h>
#include <particleview.h>
#include <timelineview.h>
#include <lazyui.h>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QShortcut>
#include <QFileDialog>
#include <QTimer>
#include <QDebug>
#include <QScrollArea>
#include <QProcessEnvironment>
#include <QResizeEvent>

namespace {

std::vector<int> createIncrementVector(int first, int count) {
    std::vector<int> result(count);
    for (unsigned int i = 0; i < result.size(); ++i) {
        result[i] = i + first;
    }
    return result;
}

std::shared_ptr<const Hist> mergeHists(
        const std::vector<std::shared_ptr<const Hist>>& hists) {
    if (hists.empty()) {
        return nullptr;
    }
    if (hists.size() == 1) {
        return hists[0];
    }
    std::vector<BinCount> binCounts(hists[0]->nDim(), BinCount("freedman"));
    return HistMerger(binCounts).merge(hists);
}

} // unnamed namespace

MainWindow::MainWindow(const std::string &layout, QWidget *parent)
  : QMainWindow(parent)
  , _histVolumeView(nullptr)
  , _histCompareView(new HistCompareView(this))
  , _queryView(new QueryView(this))
  , _timelineView(new TimelineView(this))
  , _particleView(new ParticleView(this))
  , _currTimeStep(0)
  , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // the ctrl+w shortcut to close
    auto ctrlw = new QShortcut(QKeySequence(tr("Ctrl+w")), this);
    connect(ctrlw, &QShortcut::activated, this, &MainWindow::close);
    // layout
    if ("particle" == layout) {
        createParticleLayout();
    } else if ("physical" == layout) {
        createSimpleLayout();
    } else {
        assert(false);
    }
    // open the sample dataset
    QTimer::singleShot(0, this, [this]() {
        auto home = QProcessEnvironment::systemEnvironment().value("HOME");
        this->open(home + tr("/work/histqt/data_pdf"));
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings("VIDi", "Histogram");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

void MainWindow::createParticleLayout() {
    _histVolumeView = new HistVolumeSliceView(this);
    auto vLayout = new QVBoxLayout(ui->centralWidget);
    vLayout->setMargin(0);
    vLayout->setSpacing(0);
    {
        auto gridLayout = new QGridLayout();
        gridLayout->setMargin(0);
        gridLayout->setSpacing(5);
        {
            auto openButton = new QPushButton(ui->centralWidget);
            openButton->setText(tr("Open"));
            connect(openButton, &QPushButton::clicked, this,
                    static_cast<void (MainWindow::*)()>(&MainWindow::open));

            _queryViewToggleButton = new QPushButton(ui->centralWidget);
            _queryViewToggleButton->setText(tr("Queries"));
            _queryViewToggleButton->setCheckable(true);
            connect(_queryViewToggleButton, &QPushButton::toggled,
                    this, &MainWindow::toggleQueryView);
            connect(_queryView, &QueryView::visibilityChanged,
                    this, &MainWindow::toggleQueryView);
            connect(_queryView, &QueryView::rulesChanged,
                    this, &MainWindow::setRules);

            _timelineViewToggleButton = new QPushButton(ui->centralWidget);
            _timelineViewToggleButton->setText(tr("Timeline"));
            _timelineViewToggleButton->setCheckable(true);
            _timelineViewToggleButton->setChecked(true);
            connect(_timelineViewToggleButton, &QPushButton::toggled,
                    this, &MainWindow::toggleTimelineView);
            connect(_timelineView, &TimelineView::visibilityChanged,
                    this, &MainWindow::toggleTimelineView);
            connect(_timelineView, &TimelineView::timeStepChanged,
                    this, &MainWindow::setTimeStep);

            _particleViewToggleButton = new QPushButton(ui->centralWidget);
            _particleViewToggleButton->setText(tr("Particles"));
            _particleViewToggleButton->setCheckable(true);
            connect(_particleViewToggleButton, &QPushButton::toggled,
                    this, &MainWindow::toggleParticleView);
            connect(_particleView, &ParticleView::visibilityChanged,
                    this, &MainWindow::toggleParticleView);

            auto exportParticlesButton = new QPushButton(ui->centralWidget);
            exportParticlesButton->setText(tr("Export Particles"));
            connect(exportParticlesButton, &QPushButton::clicked,
                    this, &MainWindow::exportParticles);

            int gridIndex = 0;
            gridLayout->addWidget(openButton, 0, gridIndex++);
            gridLayout->addWidget(_queryViewToggleButton, 0, gridIndex++);
            gridLayout->addWidget(_timelineViewToggleButton, 0, gridIndex++);
            gridLayout->addWidget(_particleViewToggleButton, 0, gridIndex++);
            gridLayout->addWidget(exportParticlesButton, 0, gridIndex++);
        }
        vLayout->addLayout(gridLayout, 0);
        vLayout->addWidget(_histVolumeView, 1);
        vLayout->addWidget(_timelineView, 0);
    }
}

void MainWindow::createSimpleLayout() {
    _histView = new HistView(this);
    auto physicalView = new HistVolumePhysicalView(this);
    connect(physicalView, &HistVolumePhysicalView::selectedHistsChanged, this,
            [this](std::vector<std::shared_ptr<const Hist>> hists) {
        std::shared_ptr<const Hist> merged = mergeHists(hists);
        auto histFacade = HistFacade::create(merged, merged->vars());
        auto dims = createIncrementVector(0, merged->nDim());
        _histView->setHist(histFacade, dims);
        _histView->update();
    });
    _histVolumeView = physicalView;
//    _histVolumeView = new HistVolumeNullView(this);
    _histCompareView->hide();
    _particleView->hide();

    auto vLayout = new QVBoxLayout(ui->centralWidget);
    vLayout->setMargin(5);
    vLayout->setSpacing(5);
    vLayout->addLayout([this]() {
        auto hLayout = new QHBoxLayout();
        hLayout->addLayout([this]() {
            auto vLayout = new QVBoxLayout();
            LazyUI::instance().panel()->setMinimumSize(150, 100);
            LazyUI::instance().panel()->setSizePolicy(
                    QSizePolicy::Expanding, QSizePolicy::Expanding);
            QScrollArea* scrollArea = new QScrollArea();
            scrollArea->setWidget(LazyUI::instance().panel());
            scrollArea->setFrameStyle(QFrame::NoFrame);
            scrollArea->setWidgetResizable(true);
            scrollArea->setAlignment(Qt::AlignTop);
            scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            vLayout->addWidget(scrollArea);
            vLayout->addWidget(_histView);
            return vLayout;
        }(), 1);
        hLayout->addWidget(_histVolumeView, 3);
        return hLayout;
    }(), 1);
    vLayout->addWidget(_timelineView);
    connect(_timelineView, &TimelineView::timeStepChanged,
            this, &MainWindow::setTimeStep);
    LazyUI::instance().button(tr("Open"), this, [this]() { open(); });
    readSettings();
}

void MainWindow::open()
{
    QString dir = QFileDialog::getExistingDirectory(this, "s3d_run");
    if (dir.isEmpty())
        return;
    this->open(dir);
}

void MainWindow::open(const QString &dir)
{
    if (!_data.setDir(dir.toStdString()))
        return;
    _currTimeStep = 0;
    _timelineView->setDataPool(&_data);
    _histVolumeView->setHistConfigs(_data.histConfigs());
    _histVolumeView->setDataStep(_data.step(_currTimeStep));
    _histVolumeView->update();

    auto dataStep = _data.step(_currTimeStep);
    auto name = dataStep->histConfigs()[0].name();
    auto hist = dataStep->smartVolume(name)->hist(0);
    _histView->setHist(hist, {0});
    _histView->update();

    _queryView->setHistConfigs(_data.histConfigs());
    _particleView->setVisible(false);
    glm::vec3 lower(_data.volMin()[0], _data.volMin()[1], _data.volMin()[2]);
    glm::vec3 upper(_data.volMax()[0], _data.volMax()[1], _data.volMax()[2]);
    _particleView->setBoundingBox(lower, upper);
}

void MainWindow::toggleQueryView(bool show)
{
    static bool firstTime = true;
    if (show && firstTime) {
        firstTime = false;
        _queryView->move(pos().x() + size().width(), pos().y());
    }
    _queryViewToggleButton->setChecked(show);
    _queryView->setVisible(show);
}

void MainWindow::toggleTimelineView(bool show)
{
    _timelineViewToggleButton->setChecked(show);
    _timelineView->setVisible(show);
}

void MainWindow::toggleParticleView(bool show)
{
    static bool firstTime = true;
    if (show && firstTime) {
        firstTime = false;
        _particleView->move(
                pos().x() + size().width(),
                pos().y() + size().height() - _particleView->size().height());
    }
    if (show) {
        _particles = loadTracers(
                _currTimeStep, _data.step(_currTimeStep)->selectedFlatIds());
        _particleView->setParticles(&_particles);
    }
    _particleViewToggleButton->blockSignals(true);
    _particleViewToggleButton->setChecked(show);
    _particleViewToggleButton->blockSignals(false);
    _particleView->blockSignals(true);
    _particleView->setVisible(show);
    _particleView->blockSignals(false);
}

void MainWindow::exportParticles()
{
    /// TODO: export particles within the selected histograms.
}

void MainWindow::setTimeStep(int timeStep)
{
    _currTimeStep = timeStep;
    _histVolumeView->setDataStep(_data.step(_currTimeStep));
    _histVolumeView->update();
    if (_particleView->isVisible()) {
        _particles = loadTracers(
                _currTimeStep, _data.step(_currTimeStep)->selectedFlatIds());
        _particleView->setParticles(&_particles);
        _particleView->update();
    }
}

void MainWindow::setRules(const std::vector<QueryRule> &rules)
{
    _data.setQueryRules(rules);
    if (_particleView->isVisible()) {
        _particles = loadTracers(
                _currTimeStep, _data.step(_currTimeStep)->selectedFlatIds());
        _particleView->setParticles(&_particles);
        _particleView->update();
    }
}

std::vector<Particle> MainWindow::loadTracers(
        int timeStep, const std::vector<int>& selectedHistFlatIds)
{
    // use different tracer reader when different files are present.
    TracerConfig tracerConfig = _data.tracerConfig(timeStep);
    std::shared_ptr<TracerReader> reader = TracerReader::create(tracerConfig);
    std::vector<Particle> parts = reader->read(selectedHistFlatIds);
    double tMin = std::numeric_limits<double>::max();
    double tMax = std::numeric_limits<double>::lowest();
    for (const Particle& part : parts) {
        tMin = std::min(part.temp, tMin);
        tMax = std::max(part.temp, tMax);
    }
    for (Particle& part : parts) {
        part.temp = (part.temp - tMin) / (tMax - tMin);
    }
    return parts;
}

void MainWindow::readSettings() {
    QSettings settings("VIDi", "Histogram");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

unsigned int MainWindow::nHist() const
{
    Extent dim = _data.dimHists();
    return dim[0] * dim[1] * dim[2];
}
