#ifndef _DATAPOOL_H_
#define _DATAPOOL_H_

#include <fstream>
#include <vector>
#include <memory>
#include <map>
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
            float stepInterval, bool pdfInTracerDir,
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
    float _stepInterval;
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
    bool setDir(const std::string& dir);
    std::shared_ptr<DataStep> step(int iStep);
    bool isOpen() { return m_isOpen; }
    bool setOpen( bool c ) { m_isOpen = c; return isOpen(); }
    float interval() const { return m_interval; }
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
    int m_nSteps;
    float m_interval;
    bool m_isOpen;
    bool m_pdfInTracerDir;
    GridConfig m_gridConfig;
    std::vector<HistConfig> m_histConfigs;
    std::vector<QueryRule> m_queryRules;
};

///////////////////////////////////////////////////////////////////////////////

#endif // _DATAPOOL_H_
