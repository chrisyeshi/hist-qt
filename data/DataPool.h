#ifndef _DATAPOOL_H_
#define _DATAPOOL_H_

#include <fstream>
#include <vector>
#include <memory>
#include <map>
#include "histgrid.h"
#include "histfacadegrid.h"
#include "Histogram.h"
#include "tracerreader.h"



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


template <typename T>
class TDataStep
{
public:
    TDataStep() {}
    TDataStep(const std::string& dir, const std::vector<int> dimProcs,
             const std::vector<HistConfig>& histConfigs)
      : m_dir(dir), m_dimProcs(dimProcs)
      , m_histConfigs(histConfigs) {}

public:
    const std::vector<HistConfig>& histConfigs() const { return m_histConfigs; }
    const HistConfig& histConfig(const std::string& name) const {
        auto itr = std::find_if(m_histConfigs.begin(), m_histConfigs.end(),
                [name](HistConfig histConfig){
            return histConfig.name() == name;
        });
        assert(itr != m_histConfigs.end());
        return (*itr);
    }
    std::shared_ptr<T> volume(const std::string& name) {
        if (m_data.count(name) > 0)
            return m_data[name];
        // else, try to load the data
        if (!this->load(name))
            return nullptr;
        return m_data[name];
    }

private:
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
        auto histVol = std::make_shared<T>(
                m_dir, std::string(idcstr), m_dimProcs, itr->vars);
        m_data[name] = histVol;
        return true;
    }

private:
    std::map<std::string, std::shared_ptr<T> > m_data;
    std::string m_dir;
    std::vector<int> m_dimProcs;
    std::vector<HistConfig> m_histConfigs;
};

//typedef TDataStep<HistVolume> DataStep;
typedef TDataStep<HistFacadeVolume> DataStep;


/////////////////////////////////////////////////////////////////////////////////////


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
    TracerConfig tracerConfig(int timestep) const {
        TracerConfig config(stepDir(timestep), m_dimProcs, m_dimHistsPerDomain, m_dimVoxels);
        return config;
    }

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

//    int m_nDimsHist;
//    std::vector<int> m_dimBinsPerHist;
//    std::vector<std::string> m_histVars;
//    std::vector<std::pair<double, double> > m_histRanges;
};


/////////////////////////////////////////////////////////////////////////////////////////


#endif // _DATAPOOL_H_
