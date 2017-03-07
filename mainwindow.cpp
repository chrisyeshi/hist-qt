#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <histvolumeview.h>
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

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , _histVolumeView(new HistVolumeView(this))
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
    auto vLayout = new QVBoxLayout(ui->centralWidget);
    {
        auto gridLayout = new QGridLayout();
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
    }
    // open the sample dataset
    QTimer::singleShot(0, this, [this]() {
        this->open(tr("/Users/yey/work/data/s3d_run"));
    });
}

MainWindow::~MainWindow()
{
    delete ui;
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
    _data.setDir(dir.toStdString());
    _selectedHistMask.resetToAllTrue(_data.dimHists());
    _currTimeStep = 0;
    _timelineView->setNTimeSteps(_data.numSteps());
    _histVolumeView->setHistConfigs(_data.histConfigs());
    _histVolumeView->setDataStep(_data.step(_currTimeStep));
    _histVolumeView->setSelectedHistMask(_selectedHistMask);
    _histVolumeView->update();
    _queryView->setHistConfigs(_data.histConfigs());
    _queryRules.clear();
    applyQueryRules();
    _particleView->setVisible(false);
    glm::vec3 lower(_data.volMin()[0], _data.volMin()[1], _data.volMin()[2]);
    glm::vec3 upper(_data.volMax()[0], _data.volMax()[1], _data.volMax()[2]);
    _particleView->setBoundingBox(lower, upper);
}

void MainWindow::toggleQueryView(bool show)
{
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
    if (show) {
        _particles = loadTracers(_currTimeStep, _selectedHistMask);
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
    applyQueryRules();
    _histVolumeView->setDataStep(_data.step(_currTimeStep));
    _histVolumeView->setSelectedHistMask(_selectedHistMask);
    _histVolumeView->update();
    if (_particleView->isVisible()) {
        _particles = loadTracers(_currTimeStep, _selectedHistMask);
        _particleView->setParticles(&_particles);
        _particleView->update();
    }
}

void MainWindow::setRules(const std::vector<QueryRule> &rules)
{
    _queryRules = rules;
    _data.setQueryRules(rules);
    applyQueryRules();
    _histVolumeView->setSelectedHistMask(_selectedHistMask);
    _histVolumeView->update();
    if (_particleView->isVisible()) {
        _particles = loadTracers(_currTimeStep, _selectedHistMask);
        _particleView->setParticles(&_particles);
        _particleView->update();
    }
}

void MainWindow::applyQueryRules()
{
    // seperate the rules as they are targetting different hist configs.
    std::unordered_map<std::string, std::vector<QueryRule>> histNameToRules;
    for (const QueryRule& rule : _queryRules) {
        histNameToRules[rule.histName].push_back(rule);
    }
    // reset to all true
    _selectedHistMask.resetToAllTrue();
    // for each histogram config
    for (const auto& keyValue : histNameToRules) {
        const std::string& histName = keyValue.first;
        const std::vector<QueryRule>& rules = keyValue.second;
        auto histVolume = _data.step(_currTimeStep)->volume(histName);
        // for each histogram in a histogram volume
        for (int iHist = 0; iHist < histVolume->helper().N_HIST; ++iHist) {
            auto hist = histVolume->hist(iHist);
            bool histSelected = _selectedHistMask.isSelected(iHist);
            // for each rule targetting this hist config
            for (auto rule : rules) {
                // check if the histogram is selected
                bool selected =
                        hist->checkRange(rule.intervals, rule.threshold);
                histSelected = histSelected && selected;
                if (!histSelected)
                    break;
            }
            // put it in the mask
            _selectedHistMask.setSelected(iHist, histSelected);
        }
    }
}

std::vector<Particle> MainWindow::loadTracers(
        int timeStep, const BoolMask3D &mask)
{
    std::vector<int> selectedHistFlatIds;
    for (unsigned int iHist = 0; iHist < nHist(); ++iHist)
        if (mask.isSelected(iHist))
            selectedHistFlatIds.push_back(iHist);
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
