#include "DataPool.h"
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include <unordered_set>
#include <util.h>
#include <QFileInfo>
#include <json/json.h>
#include "dataconfigreader.h"

namespace {

const std::string data_out = "/data/";
const std::string tracer_pre = "tracer-";
const std::string pdf_pre = "pdf-";
static std::shared_ptr<StatsThread> _statsThread;

bool fileExists(const std::string& filePath) {
    QFileInfo info(QString::fromStdString(filePath));
    return info.exists() && info.isFile();
}

DataPool::Stats loadStatsFromFile(const std::string& filePath) {
    Json::Value jStats;
    Json::Reader reader;
    std::ifstream fin(filePath.c_str());
    reader.parse(fin, jStats);
    DataPool::Stats stats;
    for (int iStep = 0; iStep < jStats.size(); ++iStep) {
        Json::Value jStep = jStats[iStep];
        DataStep::Stats step;
        for (auto volKey : jStep.getMemberNames()) {
            Json::Value jVolume = jStep[volKey];
            HistFacadeVolume::Stats volume;
            for (auto varKey : jVolume.getMemberNames()) {
                volume.means[varKey] = jVolume[varKey]["mean"].asFloat();
                volume.meanRanges[varKey] = {
                    jVolume[varKey]["meanRange"]["min"].asFloat(),
                    jVolume[varKey]["meanRange"]["max"].asFloat(),
                };
            }
            step[volKey] = volume;
        }
        stats.push_back(step);
    }
    return stats;
}

void saveStatsToFile(
        const DataPool::Stats& stats, const std::string& filePath) {
    Json::Value jStats;
    for (unsigned int iStep = 0; iStep < stats.size(); ++iStep) {
        const auto& step = stats[iStep];
        Json::Value jStep;
        for (auto pair : step) {
            const auto& volume = pair.second;
            Json::Value jVolume;
            for (auto vPair : volume.means) {
                const auto& key = vPair.first;
                jVolume[key]["mean"] = volume.means.at(key);
                jVolume[key]["meanRange"]["min"] = volume.meanRanges.at(key)[0];
                jVolume[key]["meanRange"]["max"] = volume.meanRanges.at(key)[1];
            }
            jStep[pair.first] = jVolume;
        }
        jStats[iStep] = jStep;
    }
    std::ofstream fout(filePath.c_str());
    assert(fout);
    Json::StyledStreamWriter writer;
    writer.write(fout, jStats);
}

} // unnamed namespace

/**
 * @brief operator <<
 * @param os
 * @param rule
 * @return
 */
std::ostream &operator<<(std::ostream &os, const QueryRule &rule)
{
    os << "Histogram Name: " << rule.histName << std::endl;
    for (auto interval : rule.intervals) {
        os << "Interval: (" << interval.lower << ", " << interval.upper << ")"
                << std::endl;
    }
    os << "Threshold: " << rule.threshold << std::endl;
    return os;
}

bool operator==(const QueryRule &a, const QueryRule &b)
{
    if (a.histName != b.histName)
        return false;
    if (fabs(a.threshold - b.threshold) > 0.0001)
        return false;
    if (a.intervals != b.intervals)
        return false;
    return true;
}

void DataLoader::initialize(std::string dir, GridConfig gridConfig,
        TimeSteps timeSteps, bool pdfInTracerDir,
        std::vector<HistConfig> configs) {
    clearAsync();
    waitForAsync();
    _dir = dir;
    _gridConfig = gridConfig;
    _timeSteps = timeSteps;
    _pdfInTracerDir = pdfInTracerDir;
    _histConfigs = configs;
}

std::shared_ptr<HistFacadeVolume> DataLoader::load(
        const HistVolumeId &histVolumeId) {
    int stepId = histVolumeId.first;
    std::string name = histVolumeId.second;
    auto itr = std::find_if(_histConfigs.begin(), _histConfigs.end(),
            [name](HistConfig histConfig){
        return histConfig.name() == name;
    });
    if (_histConfigs.end() == itr)
        return nullptr;
    int index = itr - _histConfigs.begin() + 1;
    std::string idcstr = yy::sprintf("%03d", index);
    auto histVol = ([&]() {
        if (GridConfig::GridType_UniformGrid == _gridConfig.gridType()) {
            return std::make_shared<HistFacadeVolume>(stepDir(stepId), idcstr,
                    std::vector<int>(_gridConfig.dimProcs()), itr->vars);
        } else if (GridConfig::GridType_MultiBlock == _gridConfig.gridType()) {
            return std::make_shared<HistFacadeVolume>(stepDir(stepId), idcstr,
                    _gridConfig.multiBlocks(), itr->vars);
        }
        assert(false);
    })();
    return histVol;
}

void DataLoader::processQueue()
{
    _isLoading = true;
    while (!_queue.empty()) {
        _queueMutex.lock();
        auto histVolumeId = _queue[0];
        _queue.erase(_queue.begin());
        _queueMutex.unlock();
        auto histVolume = load(histVolumeId);
        emit histVolumeLoaded(histVolumeId, histVolume);
    }
    _isLoading = false;
}

void DataLoader::asyncLoad(HistVolumeId histVolumeId)
{
    _queueMutex.lock();
    _queue.push_back(histVolumeId);
    _queueMutex.unlock();
    if (!_isLoading) {
        QMetaObject::invokeMethod(this, "processQueue", Qt::QueuedConnection);
    }
}

void DataLoader::asyncLoad(const HistVolumeIds &histVolumeIds)
{
    for (const auto& id : histVolumeIds) {
        _queueMutex.lock();
        _queue.push_back(id);
        _queueMutex.unlock();
    }
    if (!_isLoading) {
        QMetaObject::invokeMethod(this, "processQueue", Qt::QueuedConnection);
    }
}

void DataLoader::clearAsync()
{
    _queueMutex.lock();
    _queue.clear();
    _queueMutex.unlock();
}

void DataLoader::waitForAsync()
{
    while (_isLoading) {};
}

std::string DataLoader::stepDir(int iStep) const {
    auto stepStr = _timeSteps.asString(iStep);
    if (_pdfInTracerDir)
        return _dir + "/" + data_out + "/" + tracer_pre + stepStr + "/";
    return _dir + "/" + pdf_pre + stepStr + "/";
}

/**
 * @brief DataStep::DataStep
 * @param dir
 * @param dimProcs
 * @param dimHistsPerDomain
 * @param histConfigs
 */
DataStep::DataStep(
        int stepId, GridConfig gridConfig, std::vector<HistConfig> histConfigs,
        DataLoader *dataLoader, QObject *parent)
  : QObject(parent), m_stepId(stepId), m_gridConfig(gridConfig)
  , m_histConfigs(histConfigs), m_dataLoader(dataLoader)
{

}

int DataStep::nHist() const {
    return Extent(m_gridConfig.dimHists()).nElement();
}

const HistConfig &DataStep::histConfig(const std::string &name) const {
    auto itr = std::find_if(
            m_histConfigs.begin(), m_histConfigs.end(),
            [name](HistConfig histConfig) {
        return histConfig.name() == name;
    });
    assert(itr != m_histConfigs.end());
    return (*itr);
}

void DataStep::setVolume(
        std::string name, std::shared_ptr<HistFacadeVolume> volume) {
//    std::cout << "DataStep::setVolume(" << m_stepId << ", " << name << ")"
//            << std::endl;
    m_data[name] = volume;
//    /// TODO: separate into it's own function for better performance?
//    qDebug() << "nHist()" << nHist();
//    for (int iHist = 0; iHist < nHist(); ++iHist)
//        volume->hist(iHist)->setSelected(m_histMask[iHist]);
}

std::shared_ptr<HistFacadeVolume> DataStep::dumbVolume(
        const std::string &name) {
    if (m_data.count(name) <= 0) {
        return nullptr;
    }
    return m_data[name];
}

std::shared_ptr<HistFacadeVolume> DataStep::smartVolume(
        const std::string &name) {
    if (m_data.count(name) > 0)
        return m_data[name];
    /// TODO: the following is dangerous.
    emit signalLoadHistVolume({ m_stepId, name });
    assert(m_data.count(name) > 0);
//    while (m_data.count(name) == 0) {};
    return m_data[name];
}

void DataStep::setQueryRules(const std::vector<QueryRule> &rules) {
    m_queryRules = rules;
    std::unordered_set<std::string> histNames;
    for (auto& queryRule : m_queryRules) {
        histNames.insert(queryRule.histName);
    }
    for (auto itr = histNames.begin(); itr != histNames.end(); ++itr) {
        const std::string& histName = *itr;
        if (!dumbVolume(histName)) {
            auto histVol = m_dataLoader->load({ m_stepId, histName });
            assert(histVol);
            setVolume(histName, histVol);
        }
    }
    applyQueryRules();
}

std::vector<int> DataStep::selectedFlatIds() const {
    std::vector<int> flatIds;
    for (int iHist = 0; iHist < nHist(); ++iHist) {
        if (m_histMask[iHist]) {
            flatIds.push_back(iHist);
        }
    }
    return flatIds;
}

void DataStep::applyQueryRules() {
    // separate the rules as they are targetting different hist configs.
    std::unordered_map<std::string, std::vector<QueryRule>> histNameToRules;
    for (const QueryRule& rule : m_queryRules) {
        histNameToRules[rule.histName].push_back(rule);
    }
    // reset to all true
    m_histMask.resize(nHist());
    m_histMask.assign(nHist(), true);
    // for each histogram config
    for (const auto& keyValue : histNameToRules) {
        const std::string& histName = keyValue.first;
        const std::vector<QueryRule>& rules = keyValue.second;
        auto histVolume = dumbVolume(histName);
        assert(histVolume);
        // for each histogram in a histogram volume
        for (int iHist = 0; iHist < nHist(); ++iHist) {
            auto hist = histVolume->hist(iHist);
            bool histSelected = m_histMask[iHist];
            // for each rule targettting this hist config
            for (auto rule : rules) {
                // check if the histogram is selected
                bool selected =
                        hist->checkRange(rule.intervals, rule.threshold);
                histSelected = histSelected && selected;
                if (!histSelected)
                    break;
            }
            // put it in the mask
            m_histMask[iHist] = histSelected;
        }
    }
    // set selected in the hist facades
    for (auto keyValue : m_data)
        for (int iHist = 0; iHist < nHist(); ++iHist) {
            keyValue.second->hist(iHist)->setSelected(m_histMask[iHist]);
        }
    // emit signal
    emit histSelectionChanged();
}

//bool DataStep::load(const std::string &name) {
//    emit signalLoadHistVolume(m_stepId, name);

//    m_dataLoader->clearAsync();
//    m_dataLoader->waitForAsync();

//    auto histVol = m_dataLoader->load({ m_stepId, name });
//    if (!histVol)
//        return false;
//    m_data[name] = histVol;
//    /// TODO: separate into it's own function for better performance?
//    for (int iHist = 0; iHist < nHist(); ++iHist)
//        histVol->hist(iHist)->setSelected(m_histMask[iHist]);

//    for (auto histConfig : m_histConfigs) {
//        if (!m_data.count(histConfig.name()))
//            m_dataLoader->asyncLoad({ m_stepId, histConfig.name() });
//    }

//    return true;

////    auto itr = std::find_if(m_histConfigs.begin(), m_histConfigs.end(),
////            [name](HistConfig histConfig){
////        return histConfig.name() == name;
////    });
////    if (m_histConfigs.end() == itr)
////        return false;
////    int index = itr - m_histConfigs.begin() + 1;
////    char idcstr[5];
////    sprintf(idcstr, "%03d", index);
////    auto histVol = std::make_shared<HistFacadeVolume>(
////            m_dir, std::string(idcstr), m_dimProcs, itr->vars);
////    /// TODO: separate into it's own function for better performance?
////    for (int iHist = 0; iHist < nHist(); ++iHist)
////        histVol->hist(iHist)->setSelected(m_histMask[iHist]);
////    m_data[name] = histVol;
////    return true;
//}

////////////////////////////////////////////////////////////////////////////////

DataPool::DataPool()
  : m_dataLoader(new DataLoader())
  , m_isOpen(false) {
    qRegisterMetaType<DataLoader::HistVolumeId>("HistVolumeId");
    qRegisterMetaType<std::shared_ptr<HistFacadeVolume>>(
            "std::shared_ptr<HistFacadeVolume>");
    m_dataLoader->moveToThread(&m_dataLoaderThread);
    connect(&m_dataLoaderThread, &QThread::finished,
            m_dataLoader, &DataLoader::deleteLater);
    connect(m_dataLoader, &DataLoader::histVolumeLoaded,
            this, &DataPool::histVolumeLoaded);
    m_dataLoaderThread.start();
}

DataPool::~DataPool() {
    m_dataLoaderThread.quit();
    m_dataLoaderThread.wait();
}

bool DataPool::setDir(const std::string& dir)
{
    _statsThread = nullptr;
    std::shared_ptr<DataConfigReader> dataConfigReader = nullptr;
    std::ifstream mbpdf((dir + "/multi_block.config").c_str());
    std::ifstream fpdf((dir + "/pdf.config").c_str());
    if (mbpdf) {
        dataConfigReader = std::make_shared<MultiBlockConfigReader>(dir);
        m_pdfInTracerDir = false;
    } else if (fpdf) {
        dataConfigReader = std::make_shared<PdfDataConfigReader>(dir);
        m_pdfInTracerDir = false;
    } else {
        dataConfigReader = std::make_shared<S3DDataConfigReader>(dir);
        m_pdfInTracerDir = true;
    }
    if (!dataConfigReader || !dataConfigReader->read())
        return false;
    m_gridConfig = dataConfigReader->gridConfig();
    m_timeSteps = dataConfigReader->timeSteps();
    m_histConfigs = dataConfigReader->histConfigs();

    m_dir = dir;
    m_data.clear();
    m_data.resize(m_timeSteps.nSteps());

    m_dataLoader->initialize(
            m_dir, m_gridConfig, m_timeSteps, m_pdfInTracerDir, m_histConfigs);

    m_isOpen = true;
    return m_isOpen;
}

std::shared_ptr<DataStep> DataPool::step(int iStep)
{
    if (iStep >= m_timeSteps.nSteps())
        return nullptr;
    if (!m_data[iStep]) {
        m_data[iStep] = std::make_shared<DataStep>(
                iStep, m_gridConfig, m_histConfigs, m_dataLoader);
        connect(m_data[iStep].get(), &DataStep::signalLoadHistVolume,
                this, &DataPool::loadHistVolume);
//        m_data[iStep]->setQueryRules(m_queryRules);
    }
    return m_data[iStep];
}

void DataPool::stats(std::function<void(Stats)> callback) const {
    assert(m_isOpen);
    std::string filePath = m_dir + "/datastats.json";
    std::cout << filePath << std::endl;
    if (fileExists(filePath)) {
        Stats stats = loadStatsFromFile(filePath);
        callback(stats);
    } else {
        _statsThread = std::make_shared<StatsThread>();
        _statsThread->compute(
                m_dir, m_gridConfig, m_timeSteps, m_pdfInTracerDir,
                m_histConfigs, QThread::currentThread(), [=](Stats stats) {
            saveStatsToFile(stats, filePath);
            callback(stats);
        });
    }
}

const HistConfig &DataPool::histConfig(const std::string &name) const
{
    auto itr = std::find_if(m_histConfigs.begin(), m_histConfigs.end(),
            [name](HistConfig histConfig){
        return histConfig.name() == name;
    });
    assert(itr != m_histConfigs.end());
    return (*itr);
}

TracerConfig DataPool::tracerConfig(int timestep) const {
    TracerConfig config(
            stepDir(timestep), m_gridConfig.dimProcs(),
            m_gridConfig.dimHistsPerDomain(),
            m_gridConfig.dimVoxels());
    return config;
}

void DataPool::setQueryRules(const std::vector<QueryRule> &rules)
{
    // store the rules and apply it whenever new data is loaded.
    m_queryRules = rules;
    // loop through existing data steps and apply the rule.
    for (unsigned int iStep = 0; iStep < m_data.size(); ++iStep) {
        auto step = m_data[iStep];
        if (step) {
            step->setQueryRules(m_queryRules);
        }
    }
}

void DataPool::histVolumeLoaded(DataLoader::HistVolumeId histVolumeId,
        std::shared_ptr<HistFacadeVolume> histVolume) {
    int stepId = histVolumeId.first;
    std::string name = histVolumeId.second;
    auto itr = std::find_if(m_histConfigs.begin(), m_histConfigs.end(),
            [name](const HistConfig& config) {
        return config.name() == name;
    });
    if (this->step(stepId) && itr != m_histConfigs.end()) {
        this->step(stepId)->setVolume(name, histVolume);
    }
}

void DataPool::loadHistVolume(DataLoader::HistVolumeId histVolumeId) {
    int stepId = histVolumeId.first;
    std::string name = histVolumeId.second;

    m_dataLoader->clearAsync();
    m_dataLoader->waitForAsync();

    auto histVol = m_dataLoader->load(histVolumeId);
    assert(histVol);
    this->step(stepId)->setVolume(name, histVol);

    QTimer::singleShot(0, this, [=]() {
        int bufferRadius = 40;
        // remove steps that are too far away from the selected step
        for (int iStep = 0; iStep < stepId - bufferRadius; ++iStep) {
            m_data[iStep] = nullptr;
        }
        for (int iStep = stepId + bufferRadius + 1;
                iStep < m_timeSteps.nSteps(); ++iStep) {
            m_data[iStep] = nullptr;
        }
        // preload nearby steps of the same volume name
        for (int iStep = std::max(stepId - bufferRadius, 0);
                iStep <=
                    std::min(stepId + bufferRadius, m_timeSteps.nSteps() - 1);
                ++iStep) {
            if (!this->step(iStep)->dumbVolume(name))
                m_dataLoader->asyncLoad({ iStep, name });
        }
        // preload different volumes in the same step
        for (auto histConfig : m_histConfigs) {
            if (!this->step(stepId)->dumbVolume(histConfig.name()))
                m_dataLoader->asyncLoad({ stepId, histConfig.name() });
        }
    });

//    for (auto histConfig : m_histConfigs) {
//        if (!m_data[stepId])

//        if (!m_data.count(stepId))
//        if (!m_data.count(histConfig.name()))
//            m_dataLoader->asyncLoad({ m_stepId, histConfig.name() });
//    }

}

std::string DataPool::stepDir(int iStep) const
{
    return m_dataLoader->stepDir(iStep);
}
