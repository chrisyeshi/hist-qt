#include "DataPool.h"
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace {
    const std::string s3d_in = "/input/s3d.in";
    const std::string tracer_in = "/input/tracer.in";
    const std::string pdf_in = "/input/histogram.in";
    const std::string data_out = "/data/";
    const std::string tracer_pre = "tracer-";
}

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

/**
 * @brief HistConfig::name
 * @return
 */
std::string HistConfig::name() const
{
    std::string ret = vars[0];
    for (unsigned int i = 1; i < vars.size(); ++i) {
        ret += "-" + vars[i];
    }
    return ret;
}

bool HistConfig::load(std::istream &in)
{
    std::string line;
    in >> nDim; std::getline(in, line);
    if (!in) return false;
    vars.resize(nDim);
    rangeMethods.resize(nDim);
    nBins.resize(nDim);
    mins.resize(nDim);
    maxs.resize(nDim);
    /// TODO: actually recognize the comments instead of using getline.
    for (auto iDim = 0; iDim < nDim; ++iDim)
        in >> vars[iDim] >> nBins[iDim] >> rangeMethods[iDim] >> mins[iDim] >>
                maxs[iDim];
    std::getline(in, line);
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////

bool DataPool::setDir(const std::string& dir)
{
    std::ifstream fs3d(    ( dir + s3d_in    ).c_str() );
    std::ifstream ftracer( ( dir + tracer_in ).c_str() );
    std::ifstream fhist(   ( dir + pdf_in    ).c_str() );

    if (!fs3d || !ftracer || !fhist)
        return false;

    printf( "%s\n", ( dir + s3d_in    ).c_str() );
    printf( "%s\n", ( dir + tracer_in ).c_str() );
    printf( "%s\n", ( dir + pdf_in    ).c_str() );

    std::string line;
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    // m_dimVoxels
    m_dimVoxels.resize(3);
    fs3d >> m_dimVoxels[0]; std::getline(fs3d, line);
    fs3d >> m_dimVoxels[1]; std::getline(fs3d, line);
    fs3d >> m_dimVoxels[2]; std::getline(fs3d, line);
    // m_dimProcs
    m_dimProcs.resize(3);
    fs3d >> m_dimProcs[0]; std::getline(fs3d, line);
    fs3d >> m_dimProcs[1]; std::getline(fs3d, line);
    fs3d >> m_dimProcs[2]; std::getline(fs3d, line);

    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    // m_nSteps and m_interval
    int nTimes, nTimesPerField;
    fs3d >> nTimes;
    std::getline(fs3d, line);
    fs3d >> nTimesPerField;
    std::getline(fs3d, line);
    std::getline(fs3d, line);

    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);

    std::getline(fs3d, line);
    std::getline(fs3d, line);
    std::getline(fs3d, line);

    m_volMin.resize(3);
    m_volMax.resize(3);
    fs3d >> m_volMin[0]; m_volMin[0] /= 100.f; std::getline(fs3d, line);
    fs3d >> m_volMin[1]; m_volMin[1] /= 100.f; std::getline(fs3d, line);
    fs3d >> m_volMin[2]; m_volMin[2] /= 100.f; std::getline(fs3d, line);
    fs3d >> m_volMax[0]; m_volMax[0] /= 100.f; std::getline(fs3d, line);
    fs3d >> m_volMax[1]; m_volMax[1] /= 100.f; std::getline(fs3d, line);
    fs3d >> m_volMax[2]; m_volMax[2] /= 100.f; std::getline(fs3d, line);
//    std::cout << "volmin: " << m_volMin[0] << ", " << m_volMin[1] << ", " << m_volMin[2] << std::endl;
//    std::cout << "volmax: " << m_volMax[0] << ", " << m_volMax[1] << ", " << m_volMax[2] << std::endl;

    float freqTracer;
    std::getline(ftracer, line);
    std::getline(ftracer, line);
    std::getline(ftracer, line);
    std::getline(ftracer, line);
    ftracer >> freqTracer;
    std::getline(ftracer, line);
    int nTimesPerStep = nTimesPerField * freqTracer;

    m_nSteps = nTimes / nTimesPerStep;
    m_interval = 0.5e-8 * nTimesPerStep;

    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    fhist >> m_dimHistsPerDomain[0] >> m_dimHistsPerDomain[1] >> m_dimHistsPerDomain[2]; std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    std::getline(fhist, line);
    while(fhist) {
        HistConfig histConfig;
        if (histConfig.load(fhist))
            m_histConfigs.push_back(histConfig);
    }

    m_dir = dir;
    m_data.clear();
    m_data.resize(m_nSteps);

    m_isOpen = true;
    return m_isOpen;
}

std::shared_ptr<DataStep> DataPool::step(int iStep)
{
    if (iStep >= m_nSteps)
        return nullptr;
    if (!m_data[iStep]) {
        m_data[iStep] =
                std::make_shared<DataStep>(stepDir(iStep), m_dimProcs,
                    m_dimHistsPerDomain, m_histConfigs);
        m_data[iStep]->setQueryRules(m_queryRules);
    }
    return m_data[iStep];
}

Extent DataPool::dimHists() const
{
    Extent extent(
            m_dimProcs[0] * m_dimHistsPerDomain[0],
            m_dimProcs[1] * m_dimHistsPerDomain[1],
            m_dimProcs[2] * m_dimHistsPerDomain[2]);
    return extent;
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
    TracerConfig config(stepDir(timestep), m_dimProcs, m_dimHistsPerDomain, m_dimVoxels);
    return config;
}

void DataPool::setQueryRules(const std::vector<QueryRule> &rules)
{
    // store the rules and apply it whenever new data is loaded.
    m_queryRules = rules;
    // loop through existing data steps and apply the rule.
    for (auto step : m_data) {
        if (step)
            step->setQueryRules(m_queryRules);
    }
}

std::string DataPool::stepDir(int iStep) const
{
    float outstep = m_interval * (iStep + 1);
    char stepStr[100];
    sprintf(stepStr, "%.4E", outstep);
    std::string dir = m_dir + "/" + data_out + "/" + tracer_pre + stepStr + "/";
//    std::cout << "Tracer Dir: " << dir << std::endl;
    return dir;
}
