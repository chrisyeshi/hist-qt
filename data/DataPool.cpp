#include "DataPool.h"
#include <sstream>
#include <algorithm>
#include <cstdio>
#include "dataconfigreader.h"

namespace {
    const std::string data_out = "/data/";
    const std::string tracer_pre = "tracer-";
    const std::string pdf_pre = "pdf-";
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
    std::string dimension;
    in >> dimension;
    if (dimension == "dimension") {
        in >> nDim;
    } else {
        nDim = atoi(dimension.c_str());
    }
    std::getline(in, line);
    if (!in) return false;
    vars.resize(nDim);
    rangeMethods.resize(nDim);
    nBins.resize(nDim);
    mins.resize(nDim);
    maxs.resize(nDim);
    /// TODO: actually recognize the comments instead of using getline.
    for (auto iDim = 0; iDim < nDim; ++iDim) {
        in >> vars[iDim] >> nBins[iDim] >> rangeMethods[iDim] >> mins[iDim] >>
                maxs[iDim];
        std::getline(in, line);
    }
    return true;
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
 * @brief DataStep::DataStep
 * @param dir
 * @param dimProcs
 * @param dimHistsPerDomain
 * @param histConfigs
 */
DataStep::DataStep(std::string dir, std::vector<int> dimProcs,
        std::vector<int> dimHistsPerDomain, std::vector<HistConfig> histConfigs,
        QObject *parent)
  : QObject(parent), m_dir(dir), m_dimProcs(dimProcs)
  , m_dimHistsPerDomain(dimHistsPerDomain)
  , m_histConfigs(histConfigs), m_histMask(nHist(), true)
{

}

////////////////////////////////////////////////////////////////////////////////

bool DataPool::setDir(const std::string& dir)
{
    std::shared_ptr<DataConfigReader> dataConfigReader = nullptr;
    std::ifstream fpdf((dir + "/pdf.config").c_str());
    if (fpdf) {
        dataConfigReader = std::make_shared<PdfDataConfigReader>(dir);
        m_pdfInTracerDir = false;
    } else {
        dataConfigReader = std::make_shared<S3DDataConfigReader>(dir);
        m_pdfInTracerDir = true;
    }
    if (!dataConfigReader || !dataConfigReader->read())
        return false;
    m_dimVoxels = dataConfigReader->dimVoxels();
    m_dimProcs = dataConfigReader->dimProcs();
    int nTimes = dataConfigReader->nTimes();
    int nTimesPerField = dataConfigReader->nTimesPerField();
    m_volMin = dataConfigReader->volMin();
    m_volMax = dataConfigReader->volMax();
    float freqTracer = dataConfigReader->freqTracer();
    m_dimHistsPerDomain = dataConfigReader->dimHistsPerDomain();
    m_histConfigs = dataConfigReader->histConfigs();

    int nTimesPerStep = nTimesPerField * freqTracer;
    m_nSteps = nTimes / nTimesPerStep;
    m_interval = 0.5e-8 * nTimesPerStep;

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
    TracerConfig config(
            stepDir(timestep), m_dimProcs, m_dimHistsPerDomain, m_dimVoxels);
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
    if (m_pdfInTracerDir)
        return m_dir + "/" + data_out + "/" + tracer_pre + stepStr + "/";
    else
        return m_dir + "/" + pdf_pre + stepStr + "/";
}
