#include "histreader.h"
#include <fstream>
#include "histgrid.h"
#include "histfacadegrid.h"

namespace {
    const std::string pdfhelper_pre = "pdfhelper.";
    const std::string pdfids_pre = "pdfids.";
    const std::string pdfoffsets_pre = "pdfoffsets.";
    const std::string pdfvalues_pre = "pdfvalues.";
}

/**
 * @brief HistDomainReaderManyFiles::read
 * @param histHelper
 * @param hists
 */
void HistDomainReaderManyFiles::read(
        HistHelper &histHelper, std::vector<std::shared_ptr<Hist> > &hists)
{
    std::vector<int> nbins(3);
    std::vector<double> mins(3), maxs(3), logBases(3);
    histHelper = readHelper(nbins, mins, maxs, logBases);
    std::vector<int> offsets = readOffsets();
    std::vector<int> binIds = readBinIds();
    std::vector<double> values = readValues();
    offsets.push_back(binIds.size());
    hists.resize(histHelper.N_HIST);
    for (int iHist = 0; iHist < histHelper.N_HIST; ++iHist) {
        std::vector<int> localBinIds(
                    binIds.begin() + offsets[iHist],
                    binIds.begin() + offsets[iHist + 1]);
        std::vector<float> localValues(
                    values.begin() + offsets[iHist],
                    values.begin() + offsets[iHist + 1]);
        /// TODO: combine nbins[0], nbins[1], nbins[3]?
        hists[iHist] = Hist3D::create(
                    nbins[0], nbins[1], nbins[1],
                    mins, maxs, logBases, localBinIds, localValues, m_vars);
    }
}

HistHelper HistDomainReaderManyFiles::readHelper(
        std::vector<int>& nbins, std::vector<double>& mins,
        std::vector<double>& maxs, std::vector<double>& logBases)
{
    HistHelper ret;
    std::string path = m_dir + "/" + pdfhelper_pre + m_iProcStr;
    std::ifstream fin(path.c_str(), std::ios::in);
    assert(fin);
    fin >> ret;
    fin >> nbins[0] >> nbins[1] >> nbins[2];
    fin >> mins[0] >> mins[1] >> mins[2];
    fin >> maxs[0] >> maxs[1] >> maxs[2];
    fin >> logBases[0] >> logBases[1] >> logBases[2];
    return ret;
}

std::vector<int> HistDomainReaderManyFiles::readOffsets()
{
    std::string path = m_dir + "/" + pdfoffsets_pre + m_iProcStr;
    std::ifstream fin(path.c_str(), std::ios::in | std::ios::binary);
    assert(fin);
    fin.seekg(0, fin.end);
    int nBytes = fin.tellg();
    fin.seekg(0, fin.beg);
    std::vector<int> offsets(nBytes / sizeof(int));
    fin.read(reinterpret_cast<char*>(offsets.data()), nBytes);
    return offsets;
}

std::vector<int> HistDomainReaderManyFiles::readBinIds()
{
    std::string path = m_dir + "/" + pdfids_pre + m_iProcStr;
    std::ifstream fin(path.c_str(), std::ios::in | std::ios::binary);
    assert(fin);
    fin.seekg(0, fin.end);
    int nBytes = fin.tellg();
    fin.seekg(0, fin.beg);
    std::vector<int> ids(nBytes / sizeof(int));
    fin.read(reinterpret_cast<char*>(ids.data()), nBytes);
    return ids;
}

std::vector<double> HistDomainReaderManyFiles::readValues()
{
    std::string path = m_dir + "/" + pdfvalues_pre + m_iProcStr;
    std::ifstream fin(path.c_str(), std::ios::in | std::ios::binary);
    assert(fin);
    fin.seekg(0, fin.end);
    int nBytes = fin.tellg();
    fin.seekg(0, fin.beg);
    std::vector<double> values(nBytes / sizeof(double));
    fin.read(reinterpret_cast<char*>(values.data()), nBytes);
    return values;
}

bool HistMetaReader::readFrom(std::istream& fin) {
    fin.read(reinterpret_cast<char*>(&ndim), sizeof(int));
    fin.read(reinterpret_cast<char*>(&ngridx), sizeof(int));
    fin.read(reinterpret_cast<char*>(&ngridy), sizeof(int));
    fin.read(reinterpret_cast<char*>(&ngridz), sizeof(int));
    fin.read(reinterpret_cast<char*>(&nhistx), sizeof(int));
    fin.read(reinterpret_cast<char*>(&nhisty), sizeof(int));
    fin.read(reinterpret_cast<char*>(&nhistz), sizeof(int));
    if (!fin) {
        return false;
    }
    logbases.resize(ndim);
    fin.read(reinterpret_cast<char*>(logbases.data()),
            sizeof(double) * ndim);
    return true;
}

void HistReaderPacked::readFrom(std::istream& fin, int ndim,
        std::vector<double> logbases, std::vector<std::string> vars) {
    int issparse, nnonemptybins;
    std::vector<double> mins(ndim);
    std::vector<double> maxs(ndim);
    std::vector<int> nbins(ndim);
    double percentinrange;
    fin.read(reinterpret_cast<char*>(&issparse), sizeof(int));
    fin.read(reinterpret_cast<char*>(mins.data()), sizeof(double) * ndim);
    fin.read(reinterpret_cast<char*>(maxs.data()), sizeof(double) * ndim);
    fin.read(reinterpret_cast<char*>(nbins.data()), sizeof(int) * ndim);
    fin.read(reinterpret_cast<char*>(&percentinrange), sizeof(double));
    fin.read(reinterpret_cast<char*>(&nnonemptybins), sizeof(int));
    int bufferSize = -1;
    if (issparse == 1) {
        bufferSize = 2 * nnonemptybins;
    } else {
        bufferSize = 1;
        for (auto i = 0; i < ndim; ++i)
            bufferSize *= nbins[i];
    }
    std::vector<int> buffer(bufferSize);
    fin.read(reinterpret_cast<char*>(buffer.data()),
            bufferSize * sizeof(int));

    hist = std::shared_ptr<Hist>(
            Hist::fromBuffer(issparse == 1, ndim, nbins, mins, maxs, logbases,
                vars, buffer));
}

/**
 * @brief HistDomainReaderPacked::read
 * @param histHelper
 * @param hists
 */
void HistDomainReaderPacked::read(
        HistHelper &histHelper, std::vector<std::shared_ptr<Hist> > &hists)
{
    std::ifstream fin(m_dir + "/pdfs-" + m_name + "." + m_iProcStr);
    assert(fin);

    HistMetaReader meta;
    meta.readFrom(fin);
    histHelper.n_vx = meta.ngridx;
    histHelper.n_vy = meta.ngridy;
    histHelper.n_vz = meta.ngridz;
    histHelper.nh_x = meta.nhistx;
    histHelper.nh_y = meta.nhisty;
    histHelper.nh_z = meta.nhistz;
    histHelper.N_HIST = meta.nhistx * meta.nhisty * meta.nhistz;

    /// TODO: take care the last newline in the histogram.in file.
    hists.resize(histHelper.N_HIST);
    for (auto iHist = 0; iHist < histHelper.N_HIST; ++iHist) {
        HistReaderPacked histReader;
        histReader.readFrom(fin, meta.ndim, meta.logbases, m_vars);
        hists[iHist] = histReader.hist;
    }
}

/**
 * @brief HistYColumnReader::read
 * @return
 */
std::vector<std::shared_ptr<HistDomain>> HistYColumnReader::read() const
{
    std::vector<std::shared_ptr<HistDomain>> histDomains;
    auto filename = m_dir + "/pdfs-ycolumn-" + m_name + "." + m_iYColumnStr;
    std::ifstream fin(filename, std::ios::binary);
    assert(fin);
    while (fin) {
        // domain meta
        HistMetaReader meta;
        meta.readFrom(fin);
        HistHelper histHelper;
        histHelper.n_vx = meta.ngridx;
        histHelper.n_vy = meta.ngridy;
        histHelper.n_vz = meta.ngridz;
        histHelper.nh_x = meta.nhistx;
        histHelper.nh_y = meta.nhisty;
        histHelper.nh_z = meta.nhistz;
        histHelper.N_HIST = meta.nhistx * meta.nhisty * meta.nhistz;
        // loop to read each histograms
        std::vector<std::shared_ptr<Hist>> hists(histHelper.N_HIST);
        for (int iHist = 0; iHist < histHelper.N_HIST; ++iHist) {
            HistReaderPacked histReader;
            histReader.readFrom(fin, meta.ndim, meta.logbases, m_vars);
            hists[iHist] = histReader.hist;
        }
        // construct the hist domain
        auto histDomain = std::make_shared<HistDomain>(histHelper, hists);
        histDomains.push_back(histDomain);
    }
    return histDomains;
}

/**
 * @brief HistFacadeYColumnReader::read
 * @return
 */
std::vector<std::shared_ptr<HistFacadeDomain>>
        HistFacadeYColumnReader::read() const {
    std::vector<std::shared_ptr<HistFacadeDomain>> histDomains;
    auto filename = m_dir + "/pdfs-ycolumn-" + m_name + "." + m_iYColumnStr;
    HistMetaReader meta;
    std::ifstream fin(filename, std::ios::binary);
    assert(fin);
    while (meta.readFrom(fin)) {
        HistHelper histHelper;
        histHelper.n_vx = meta.ngridx;
        histHelper.n_vy = meta.ngridy;
        histHelper.n_vz = meta.ngridz;
        histHelper.nh_x = meta.nhistx;
        histHelper.nh_y = meta.nhisty;
        histHelper.nh_z = meta.nhistz;
        histHelper.N_HIST = meta.nhistx * meta.nhisty * meta.nhistz;
        // loop to read each histograms
        std::vector<std::shared_ptr<HistFacade>> hists(histHelper.N_HIST);
        for (int iHist = 0; iHist < histHelper.N_HIST; ++iHist) {
            HistReaderPacked histReader;
            histReader.readFrom(fin, meta.ndim, meta.logbases, m_vars);
            hists[iHist] = HistFacade::create(histReader.hist, m_vars);
        }
        // construct the hist domain
        auto histDomain = std::make_shared<HistFacadeDomain>(histHelper, hists);
        histDomains.push_back(histDomain);
    }
    return histDomains;
}
