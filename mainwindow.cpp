#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <histvolumesliceview.h>
#include "histvolumephysicalview.h"
#include <histcompareview.h>
#include <queryview.h>
#include <particleview.h>
#include <timelineview.h>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QShortcut>
#include <QFileDialog>
#include <QTimer>
#include <QDebug>
#include <QProcessEnvironment>
#include <QResizeEvent>

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
    _histVolumeView = new HistVolumePhysicalView(this);
//    _histVolumeView = new HistVolumeNullView(this);
    _histCompareView->hide();
    _particleView->hide();

    auto vLayout = new QVBoxLayout(ui->centralWidget);
    vLayout->setMargin(0);
    vLayout->setSpacing(0);
    vLayout->addWidget(_histVolumeView);
    vLayout->addWidget(_timelineView);
    connect(_timelineView, &TimelineView::timeStepChanged,
            this, &MainWindow::setTimeStep);
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

unsigned int MainWindow::nHist() const
{
    Extent dim = _data.dimHists();
    return dim[0] * dim[1] * dim[2];
}
