#ifndef _DATAPOOL_H_
#define _DATAPOOL_H_

#include <fstream>
#include <vector>
#include <memory>
#include <map>
#include <QObject>
#include "histgrid.h"
#include "histfacadegrid.h"
#include "Histogram.h"
#include "tracerreader.h"

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

/**
 * @brief The HistConfig struct
 */
struct HistConfig {
    int nDim;
    std::vector<std::string> vars;
    std::vector<std::string> rangeMethods;
    std::vector<std::string> nBins;
    std::vector<double> mins;
    std::vector<double> maxs;

    std::string name() const;
    bool load(std::istream& in);
};

/**
 *
 */
class DataStep : public QObject
{
    Q_OBJECT
public:
    explicit DataStep(QObject* parent = 0) : QObject(parent) {}
    DataStep(std::string dir, std::vector<int> dimProcs,
            std::vector<int> dimHistsPerDomain,
            std::vector<HistConfig> histConfigs, QObject* parent = 0);

signals:
    void histSelectionChanged();

public:
    int nHist() const {
        int prod = 1;
        for (unsigned int iDim = 0; iDim < m_dimProcs.size(); ++iDim) {
            prod *= m_dimProcs[iDim] * m_dimHistsPerDomain[iDim];
        }
        return prod;
    }
    const std::vector<HistConfig>& histConfigs() const { return m_histConfigs; }
    const HistConfig& histConfig(const std::string& name) const {
        auto itr = std::find_if(m_histConfigs.begin(), m_histConfigs.end(),
                [name](HistConfig histConfig) {
            return histConfig.name() == name;
        });
        assert(itr != m_histConfigs.end());
        return (*itr);
    }
    std::shared_ptr<HistFacadeVolume> volume(const std::string& name) {
        if (m_data.count(name) > 0)
            return m_data[name];
        // else, try to load the data
        if (!this->load(name))
            return nullptr;
        return m_data[name];
    }
    void setQueryRules(const std::vector<QueryRule>& rules) {
        m_queryRules = rules;
        applyQueryRules();
    }
    std::vector<int> selectedFlatIds() const {
        std::vector<int> flatIds;
        for (int iHist = 0; iHist < nHist(); ++iHist)
            if (m_histMask[iHist])
                flatIds.push_back(iHist);
        return flatIds;
    }

private:
    void applyQueryRules() {
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
            auto histVolume = volume(histName);
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
    bool load(const std::string& name) {
        auto itr = std::find_if(m_histConfigs.begin(), m_histConfigs.end(),
                [name](HistConfig histConfig){
            return histConfig.name() == name;
        });
        if (m_histConfigs.end() == itr)
            return false;
        int index = itr - m_histConfigs.begin() + 1;
        char idcstr[5];
        sprintf(idcstr, "%03d", index);
        auto histVol = std::make_shared<HistFacadeVolume>(
                m_dir, std::string(idcstr), m_dimProcs, itr->vars);
        /// TODO: separate into it's own function for better performance?
        for (int iHist = 0; iHist < nHist(); ++iHist)
            histVol->hist(iHist)->setSelected(m_histMask[iHist]);
        m_data[name] = histVol;
        return true;
    }

private:
    std::map<std::string, std::shared_ptr<HistFacadeVolume>> m_data;
    std::string m_dir;
    std::vector<int> m_dimProcs, m_dimHistsPerDomain;
    std::vector<HistConfig> m_histConfigs;
    std::vector<QueryRule> m_queryRules;
    std::vector<bool> m_histMask;
};

/////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The DataPool class
 */
class DataPool
{
public:
    DataPool()
      : m_isOpen( false )
      , m_dimHistsPerDomain(3)
      , m_volMin(3, 1.f), m_volMax(3, 1.f) {}
    ~DataPool() {}

public:
    bool setDir(const std::string& dir);
    std::shared_ptr<DataStep> step(int iStep);
    bool isOpen() { return m_isOpen; }
    bool setOpen( bool c ) { m_isOpen = c; return isOpen(); }
    int numSteps() { return m_data.size(); }
    const std::vector<float>& volMin() const { return m_volMin; }
    const std::vector<float>& volMax() const { return m_volMax; }
    Extent dimHists() const;
    const std::vector<int>& dimHistsPerDomain() const { return m_dimHistsPerDomain; }
    const std::vector<HistConfig>& histConfigs() const { return m_histConfigs; }
    const HistConfig& histConfig(const std::string& name) const;
    TracerConfig tracerConfig(int timestep) const;
    void setQueryRules(const std::vector<QueryRule>& rules);

private:
    std::string stepDir(int iStep) const;

private:
    std::vector<std::shared_ptr<DataStep> > m_data;
    std::string m_dir;
    int m_nSteps;
    float m_interval;
    std::vector<int> m_dimVoxels;
    std::vector<int> m_dimProcs;
    bool m_isOpen;
    std::vector<int> m_dimHistsPerDomain;
    std::vector<float> m_volMin, m_volMax;
    std::vector<HistConfig> m_histConfigs;
    std::vector<QueryRule> m_queryRules;
};


/////////////////////////////////////////////////////////////////////////////////////////


#endif // _DATAPOOL_H_
