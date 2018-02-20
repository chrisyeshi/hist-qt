#ifndef _DATAPOOL_H_
#define _DATAPOOL_H_

#include <fstream>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include "histgrid.h"
#include "histfacadegrid.h"
#include "Histogram.h"
#include "tracerreader.h"
#include "dataconfigreader.h"

/**
 * @brief The QueryRule class
 */
class QueryRule {
public:
    bool isEmpty() const { return histName.empty(); }

public:
    std::string histName;
    std::vector<Interval<float>> intervals;
    float threshold;
};
std::ostream& operator<<(std::ostream& os, const QueryRule& rule);
bool operator==(const QueryRule& a, const QueryRule& b);

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

class DataLoader : public QObject {
    Q_OBJECT
public:
    typedef std::pair<int,std::string> HistVolumeId;
    typedef std::vector<HistVolumeId> HistVolumeIds;

public:
    DataLoader() {}
    void initialize(std::string dir, GridConfig gridConfig,
            TimeSteps timeSteps, bool pdfInTracerDir,
            std::vector<HistConfig> configs);

public slots:
    std::string stepDir(int iStep) const;
    std::shared_ptr<HistFacadeVolume> load(const HistVolumeId& histVolumeId);
    void processQueue();

public:
    void asyncLoad(HistVolumeId histVolumeId);
    void asyncLoad(const HistVolumeIds& histVolumeIds);
    void clearAsync();
    void waitForAsync();

signals:
    void histVolumeLoaded(HistVolumeId, std::shared_ptr<HistFacadeVolume>);

private:
    QMutex _queueMutex;
    HistVolumeIds _queue;
    bool _isLoading = false;
    std::string _dir;
    GridConfig _gridConfig;
    TimeSteps _timeSteps;
    bool _pdfInTracerDir;
    std::vector<HistConfig> _histConfigs;
};

///////////////////////////////////////////////////////////////////////////////

/**
 *
 */
class DataStep : public QObject
{
    Q_OBJECT
public:
    explicit DataStep(QObject* parent = 0) : QObject(parent) {}
    DataStep(int stepId, GridConfig gridConfig,
            std::vector<HistConfig> histConfigs, DataLoader* dataLoader,
            QObject* parent = 0);

signals:
    void histSelectionChanged();
    void signalLoadHistVolume(DataLoader::HistVolumeId);

public:
    typedef std::map<std::string, HistFacadeVolume::Stats> Stats;

public:
    int nHist() const;
    const std::vector<HistConfig>& histConfigs() const { return m_histConfigs; }
    const HistConfig& histConfig(const std::string& name) const;
    void setVolume(std::string name, std::shared_ptr<HistFacadeVolume> volume);
    std::shared_ptr<HistFacadeVolume> dumbVolume(const std::string& name);
    std::shared_ptr<HistFacadeVolume> smartVolume(const std::string& name);
    void setQueryRules(const std::vector<QueryRule>& rules);
    std::vector<int> selectedFlatIds() const;

private:
    void applyQueryRules();
//    bool load(const std::string& name);

private:
    std::map<std::string, std::shared_ptr<HistFacadeVolume>> m_data;
    int m_stepId;
    GridConfig m_gridConfig;
    std::vector<HistConfig> m_histConfigs;
    std::vector<QueryRule> m_queryRules;
    std::vector<bool> m_histMask;
    DataLoader* m_dataLoader;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief The DataPool class
 */
class DataPool : public QObject
{
    Q_OBJECT
public:
    DataPool();
    ~DataPool();

public:
    typedef std::vector<DataStep::Stats> Stats;
    void stats(std::function<void(Stats)> callback) const;

public:
    bool setDir(const std::string& dir);
    std::shared_ptr<DataStep> step(int iStep);
    bool isOpen() { return m_isOpen; }
    bool setOpen( bool c ) { m_isOpen = c; return isOpen(); }
    std::string timeStepStr(int iStep) const {
        return m_timeSteps.asString(iStep);
    }
    double timeStepDouble(int iStep) const {
        return m_timeSteps.asDouble(iStep);
    }
    int numSteps() const { return m_data.size(); }
    std::vector<float> volMin() const {
        return m_gridConfig.physicalBoundingBox().lower();
    }
    std::vector<float> volMax() const {
        return m_gridConfig.physicalBoundingBox().upper();
    }
    Extent dimHists() const;
    std::vector<int> dimHistsPerDomain() const {
        return m_gridConfig.dimHistsPerDomain();
    }
    const std::vector<HistConfig>& histConfigs() const { return m_histConfigs; }
    const HistConfig& histConfig(const std::string& name) const;
    TracerConfig tracerConfig(int timestep) const;
    void setQueryRules(const std::vector<QueryRule>& rules);

public slots:
    void histVolumeLoaded(DataLoader::HistVolumeId histVolumeId,
            std::shared_ptr<HistFacadeVolume> histVolume);
    void loadHistVolume(DataLoader::HistVolumeId histVolumeId);

private:
    std::string stepDir(int iStep) const;

private:
    QThread m_dataLoaderThread;
    DataLoader* m_dataLoader;
    std::vector<std::shared_ptr<DataStep> > m_data;
    std::string m_dir;
    bool m_isOpen;
    bool m_pdfInTracerDir;
    GridConfig m_gridConfig;
    TimeSteps m_timeSteps;
    std::vector<HistConfig> m_histConfigs;
    std::vector<QueryRule> m_queryRules;
};

/**
 * @brief The StatsThread class
 */
class StatsThread : public QThread {
    Q_OBJECT
public:
    StatsThread(QObject *parent = nullptr) : QThread(parent) {}
    virtual ~StatsThread() {
        _mutex.lock();
        _abort = true;
        _condition.wakeOne();
        _mutex.unlock();
        wait();
    }

public:
    void compute(std::string dir, GridConfig gridConfig, TimeSteps timeSteps,
            bool pdfInTracerDir, std::vector<HistConfig> histConfigs,
            QObject* context, std::function<void(DataPool::Stats)> callback) {
        QMutexLocker locker(&_mutex);
        _dir = dir;
        _gridConfig = gridConfig;
        _timeSteps = timeSteps;
        _pdfInTracerDir = pdfInTracerDir;
        _histConfigs = histConfigs;
        _context = context;
        _callback = callback;
        if (!isRunning()) {
            start(LowPriority);
        } else {
            _restart = true;
            _condition.wakeOne();
        }
    }

protected:
    virtual void run() override {
        forever {
            std::shared_ptr<DataLoader> loader = std::make_shared<DataLoader>();
            loader->initialize(_dir, _gridConfig, _timeSteps, _pdfInTracerDir,
                    _histConfigs);
            DataPool::Stats dataStats;
            for (int iStep = 0; iStep < _timeSteps.nSteps(); ++iStep) {
                if (_restart) break;
                if (_abort) return;
                DataStep::Stats stepStats;
                for (int iConfig = 0; iConfig < _histConfigs.size();
                        ++iConfig) {
                    auto name = _histConfigs[iConfig].name();
                    DataLoader::HistVolumeId histVolumeId = {iStep, name};
                    auto histVolume = loader->load(histVolumeId);
                    auto statsPerVolume = histVolume->stats();
                    stepStats[name] = statsPerVolume;
                }
                dataStats.push_back(stepStats);
                QTimer::singleShot(
                        0, _context, std::bind(_callback, dataStats));
            }
            // mutex for waking the thread
            _mutex.lock();
            if (!_restart) {
                _condition.wait(&_mutex);
            }
            _restart = false;
            _mutex.unlock();
        };

    }

private:
    QMutex _mutex;
    QWaitCondition _condition;
    bool _restart = false;
    bool _abort = false;
    std::string _dir;
    GridConfig _gridConfig;
    TimeSteps _timeSteps;
    bool _pdfInTracerDir;
    std::vector<HistConfig> _histConfigs;
    QObject* _context;
    std::function<void(DataPool::Stats)> _callback;
};

///////////////////////////////////////////////////////////////////////////////

#endif // _DATAPOOL_H_
