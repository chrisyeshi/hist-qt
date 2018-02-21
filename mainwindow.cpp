#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <histview.h>
#include <histvolumesliceview.h>
#include "histvolumephysicalview.h"
#include <histcompareview.h>
#include "userstudyctrlview.h"
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
#include <QStackedLayout>
#include <QFormLayout>
#include <signupwidget.h>

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
        return std::make_shared<HistNull>();
    }
    if (hists.size() == 1) {
        return hists[0];
    }
    std::vector<BinCount> binCounts(hists[0]->nDim(), BinCount("freedman"));
    return HistMerger(binCounts).merge(hists);
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
    return std::max(lo, std::min(hi, v));
}

QStringList getHistConfigNames(const std::vector<HistConfig>& configs) {
    QStringList names;
    names.reserve(configs.size());
    for (unsigned int iConfig = 0; iConfig < configs.size(); ++iConfig) {
        names.insert(iConfig, QString::fromStdString(configs[iConfig].name()));
    }
    return names;
}

} // unnamed namespace

/**
 * @brief HistViewHolder::HistViewHolder
 * @param parent
 */
HistViewHolder::HistViewHolder(QWidget *parent)
      : QWidget(parent),
        _histView(new HistView()),
        _label(new QLabel()) {
    // size policy
    QSizePolicy policy(sizePolicy());
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    // background color
    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::white);
    setAutoFillBackground(true);
    setPalette(pal);
    // label
    _label->setWordWrap(true);
    _label->setMargin(10);
    _label->setAlignment(Qt::AlignCenter);
    // layout
    _layout = new QStackedLayout(this);
    _layout->addWidget(_histView);
    _layout->addWidget(_label);
    // signal slots
    connect(_histView,
            SIGNAL(selectedHistRangesChanged(HistRangesMap)),
            this,
            SIGNAL(selectedHistRangesChanged(HistRangesMap)));
}

void HistViewHolder::setHist(std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    _histView->setHist(histFacade, displayDims);
    showHist();
}

void HistViewHolder::setCustomVarRanges(const HistRangesMap& varRangesMap) {
    _histView->setCustomVarRanges(varRangesMap);
}

void HistViewHolder::setText(const QString &text) {
    _label->setText(text);
    showText();
}

void HistViewHolder::showText() {
    _layout->setCurrentWidget(_label);
}

void HistViewHolder::showHist() {
    _layout->setCurrentWidget(_histView);
}

void HistViewHolder::update() {
    _histView->update();
}

/**
 * @brief MainWindow::MainWindow
 * @param layout
 * @param parent
 */
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
    // keyboard shortcuts
    auto ctrlw = new QShortcut(QKeySequence(tr("Ctrl+w")), this);
    connect(ctrlw, &QShortcut::activated, this, &MainWindow::close);
    auto ctrlLeft = new QShortcut(QKeySequence(tr("ctrl+left")), this);
    connect(ctrlLeft, &QShortcut::activated, this, [this]() {
        setTimeStep(clamp(_currTimeStep - 1, 0, _data.numSteps() - 1));
    });
    auto ctrlRight = new QShortcut(QKeySequence(tr("ctrl+right")), this);
    connect(ctrlRight, &QShortcut::activated, this, [this]() {
        setTimeStep(clamp(_currTimeStep + 1, 0, _data.numSteps() - 1));
    });
    // layout
    if ("particle" == layout) {
        createParticleLayout();
    } else if ("physical" == layout) {
        createSimpleLayout();
    } else if ("user study" == layout) {
        createUserStudyLayout();
    } else {
        assert(false);
    }
    // open the sample dataset
    QTimer::singleShot(0, this, [this]() {
        auto home = QProcessEnvironment::systemEnvironment().value("HOME");
        this->open(home + tr("/work/histqt/data_pdf"));
//        this->open(home + tr("/work/cavityhists/data_pdf"));
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
//            connect(_timelineView, &TimelineView::visibilityChanged,
//                    this, &MainWindow::toggleTimelineView);
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
    _histView = new HistViewHolder(this);
    _histView->setText(tr("Please open a dataset."));
    auto physicalView = new HistVolumePhysicalView(this);
    connect(physicalView, &HistVolumePhysicalView::currHistConfigDimsChanged,
            this, [this](std::string name, std::vector<int> displayDims) {
        _timelineView->setHistConfig(_data.histConfig(name));
        _timelineView->setDisplayDims(displayDims);
        _timelineView->update();
    });
    connect(physicalView, &HistVolumePhysicalView::selectedHistIdsChanged, this,
            [this](std::string volumeName, std::vector<int> flatIds,
                std::vector<int> displayDims) {
        qInfo() << "HistVolumePhysicalView::selectedHistIdsChanged"
                << "volumeName" << QString::fromStdString(volumeName)
                << "flatIds" << QVector<int>::fromStdVector(flatIds)
                << "displayDims" << QVector<int>::fromStdVector(displayDims);
        auto volume = _data.step(_currTimeStep)->dumbVolume(volumeName);
        auto hists = yy::fp::map(flatIds, [&](int flatId) {
            return volume->hist(flatId)->hist(displayDims);
        });
        auto merged = mergeHists(hists);
        auto histFacade = HistFacade::create(merged, merged->vars());
        auto dims = createIncrementVector(0, merged->nDim());
        _histView->setHist(histFacade, dims);
        if (dims.empty()) {
            _histView->setText(
                    tr("Selected/merged histogram will be shown here."));
        }
        _histView->update();
    }, Qt::QueuedConnection);
    connect(physicalView, &HistVolumePhysicalView::customVarRangesChanged,
            this,
            [this](const HistVolumePhysicalView::HistRangesMap& varRangesMap) {
        HistView::HistRangesMap rangesMap;
        int counter = 0;
        for (auto keyValue : varRangesMap) {
            rangesMap[counter++] = keyValue.second;
        }
        _histView->setCustomVarRanges(rangesMap);
        _histView->update();
    });
    _histVolumeView = physicalView;
    _histCompareView->hide();
    _particleView->hide();
    connect(_histView, &HistViewHolder::selectedHistRangesChanged,
            this, [this](HistVolumeView::HistRangesMap histRangesMap) {
        qInfo() << "HistView brushing to select histogram ranges:";
        for (auto dimRange : histRangesMap) {
            qInfo() << dimRange.first << ":"
                    << dimRange.second[0] << dimRange.second[1];
        }
        _histVolumeView->setCustomHistRanges(histRangesMap);
    });

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
            vLayout->addWidget(scrollArea, 1);
            vLayout->addWidget(_histView);
            return vLayout;
        }(), 1);
        hLayout->addWidget(_histVolumeView, 3);
        return hLayout;
    }(), 1);
    vLayout->addWidget(_timelineView);
    connect(_timelineView, &TimelineView::timeStepChanged,
            this, [this](int timeStep) {
        qInfo() << "TimelineView::timeStepChanged" << timeStep;
        setTimeStep(timeStep);
    });
    LazyUI::instance().button(tr("Open"), this, [this]() {
        qInfo() << "openButton";
        open();
    });
    LazyUI::instance().divider("histConfigHeader");
    LazyUI::instance().labeledCombo(tr("histVolume"), tr("Histogram Volumes"),
            FluidLayout::Item::Large);
    LazyUI::instance().labeledCombo(tr("histVar"), tr("Display Variables"),
            FluidLayout::Item::Large);
    readSettings();
}

void MainWindow::createUserStudyLayout() {
    _histView = new HistViewHolder(this);
    _histView->setText(tr("Please open a dataset."));
    auto physicalView = new HistVolumePhysicalView(this);
    connect(physicalView, &HistVolumePhysicalView::selectedHistIdsChanged, this,
            [this](std::string volumeName, std::vector<int> flatIds,
                std::vector<int> displayDims) {
        qInfo() << "HistVolumePhysicalView::selectedHistIdsChanged"
                << "volumeName" << QString::fromStdString(volumeName)
                << "flatIds" << QVector<int>::fromStdVector(flatIds)
                << "displayDims" << QVector<int>::fromStdVector(displayDims);
        auto volume = _data.step(_currTimeStep)->dumbVolume(volumeName);
        auto hists = yy::fp::map(flatIds, [&](int flatId) {
            return volume->hist(flatId)->hist(displayDims);
        });
        auto merged = mergeHists(hists);
        auto histFacade = HistFacade::create(merged, merged->vars());
        auto dims = createIncrementVector(0, merged->nDim());
        _histView->setHist(histFacade, dims);
        if (dims.empty()) {
            _histView->setText(
                    tr("Selected/merged histogram will be shown here."));
        }
        _histView->update();
    }, Qt::QueuedConnection);
    _histVolumeView = physicalView;
    _histCompareView->hide();
    _particleView->hide();
    connect(_histView, &HistViewHolder::selectedHistRangesChanged,
            this, [this](HistVolumeView::HistRangesMap histRangesMap) {
        qInfo() << "HistView brushing to select histogram ranges:";
        for (auto dimRange : histRangesMap) {
            qInfo() << dimRange.first << ":"
                    << dimRange.second[0] << dimRange.second[1];
        }
        _histVolumeView->setCustomHistRanges(histRangesMap);
    });
    // the tool layout
    auto vLayout = new QVBoxLayout();
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
            vLayout->addWidget(scrollArea, 1);
            vLayout->addWidget(_histView);
            return vLayout;
        }(), 1);
        hLayout->addLayout([this]() {
            auto vLayout = new QVBoxLayout();
            vLayout->setMargin(0);
            vLayout->setSpacing(5);
            _userStudyCtrlView = new UserStudyCtrlView();
            connect(_userStudyCtrlView, &UserStudyCtrlView::toPage,
                    this, &MainWindow::toUserStudyPage);
            vLayout->addWidget(_userStudyCtrlView, 0);
            vLayout->addWidget(_histVolumeView, 3);
            return vLayout;
        }(), 3);
        return hLayout;
    }(), 1);
    vLayout->addWidget(_timelineView);
    // the stacked layout for user study
    _sLayout = new QStackedLayout();
    _sLayout->setMargin(0);
    _sLayout->setSpacing(0);
    _sLayout->addWidget([&]() {
        auto widget = new QWidget();
        widget->setAutoFillBackground(true);
        widget->setPalette([&widget]() {
            auto pal = widget->palette();
            pal.setColor(QPalette::Background, Qt::white);
            return pal;
        }());
        auto title = new QLabel(widget);
        title->setFont(QFont(tr("mono"), 50));
        title->move(150, 300);
        title->setText(tr("Welcome\n"));
        auto message = new QLabel(widget);
        message->setFont(QFont(tr("mono"), 15));
        message->setAlignment(Qt::AlignTop);
        message->setWordWrap(true);
        message->setGeometry(150, 380, 600, 600);
        message->setText(
                tr("The purpose of this user study is to gather user") +
                tr(" experience feedback in order to improve the") +
                tr(" visualization tool under development. It is a tool") +
                tr(" for scientists to explore and analyze volumetric") +
                tr(" histograms. A typical usage is to identify") +
                tr(" regions of interest in a simulation volume.") +
                tr(" The tasks in this user study are designed to be") +
                tr(" simplified versions of what the scientists will") +
                tr(" perform."));
        auto start = new QPushButton(tr("Start"), widget);
        start->move(150, 500);
        connect(start, &QPushButton::clicked,
                this, &MainWindow::toUserStudyBgPage);
        return widget;
    }());
    _sLayout->addWidget([&]() {
        SignUpWidget* signUp = new SignUpWidget();
        signUp->setAutoFillBackground(true);
        signUp->setPalette([&signUp]() {
            auto pal = signUp->palette();
            pal.setColor(QPalette::Background, Qt::white);
            return pal;
        }());
        QFormLayout* layout = static_cast<QFormLayout*>(signUp->layout());
        layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
        layout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        connect(signUp, &SignUpWidget::submit,
                this, &MainWindow::toTutorialPage);
        connect(signUp, &SignUpWidget::cancel,
                this, &MainWindow::toWelcomePage);
        return signUp;
    }());
    _sLayout->addWidget([&]() {
        auto vWidget = new QWidget();
        vWidget->setLayout(vLayout);
        return vWidget;
    }());
    //
    ui->centralWidget->setLayout(_sLayout);
    connect(_timelineView, &TimelineView::timeStepChanged,
            this, [this](int timeStep) {
        qInfo() << "TimelineView::timeStepChanged" << timeStep;
        setTimeStep(timeStep);
    });
    LazyUI::instance().button(tr("Open"), this, [this]() {
        qInfo() << "openButton";
        open();
    });
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
    _timelineView->setTimeSteps(_data.timeSteps());
    _data.stats([this](DataPool::Stats dataStats) {
        _timelineView->setStats(dataStats);
        _timelineView->update();
    });
    _histVolumeView->setHistConfigs(_data.histConfigs());
    _histVolumeView->setDataStep(_data.step(_currTimeStep));
    _histVolumeView->update();

    _queryView->setHistConfigs(_data.histConfigs());
    _particleView->setVisible(false);
    glm::vec3 lower(_data.volMin()[0], _data.volMin()[1], _data.volMin()[2]);
    glm::vec3 upper(_data.volMax()[0], _data.volMax()[1], _data.volMax()[2]);
    _particleView->setBoundingBox(lower, upper);
    // update the settings panel
    LazyUI::instance().labeledCombo(
            tr("histVolume"), getHistConfigNames(_data.histConfigs()), this,
            [this](const QString& text) {
        qInfo() << "histVolumeCombo" << text;
        _histVolumeView->setCurrHistVolume(text);
        _histVolumeView->update();
    });
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
    _timelineView->setTimeStep(_currTimeStep);
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

void MainWindow::toWelcomePage() {
    _sLayout->setCurrentIndex(0);
    qInfo() << "to welcome page";
}

void MainWindow::toUserStudyBgPage() {
    _sLayout->setCurrentIndex(1);
    qInfo() << "to participant background page";
}

void MainWindow::toTutorialPage() {
    toUserStudyPage(0);
}

void MainWindow::toUserStudyPage(int pageId) {
    HistVolumePhysicalView* volView =
            static_cast<HistVolumePhysicalView*>(_histVolumeView);
    if (0 == pageId) {
        setTimeStep(0);
        _histView->setHist(std::make_shared<HistNullFacade>(), {});
        _histView->update();
        volView->reset();
        _sLayout->setCurrentIndex(2);
        _userStudyCtrlView->setCurrentPage(0);
        qInfo() << "to features walk through page";

    } else if (17 == pageId) {
        _sLayout->setCurrentIndex(0);
        _userStudyCtrlView->setCurrentPage(0);
        qInfo() << "to welcome page";

    } else if (6 == pageId || 10 == pageId) {
        setTimeStep(0);
        _histView->setHist(std::make_shared<HistNullFacade>(), {});
        _histView->update();
        volView->reset({0, 1});
        _sLayout->setCurrentIndex(2);
        _userStudyCtrlView->setCurrentPage(pageId);
        qInfo() << "to user study page" << pageId;

    } else if (11 <= pageId && pageId <= 16) {
        _sLayout->setCurrentIndex(2);
        _userStudyCtrlView->setCurrentPage(pageId);
        qInfo() << "to user study page" << pageId;

    } else {
        setTimeStep(0);
        _histView->setHist(std::make_shared<HistNullFacade>(), {});
        _histView->update();
        volView->reset();
        _sLayout->setCurrentIndex(2);
        _userStudyCtrlView->setCurrentPage(pageId);
        qInfo() << "to user study page" << pageId;
    }
}

unsigned int MainWindow::nHist() const
{
    Extent dim = _data.dimHists();
    return dim[0] * dim[1] * dim[2];
}
