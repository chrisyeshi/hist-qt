#include "histgrid.h"
#include <fstream>
#include "histreader.h"

namespace {
bool isFileExist(const std::string &path)
{
    std::ifstream f(path.c_str());
    return f.good();
}
} // namespace

//////////////////////////////////////////////////////////////////////////////////////
// HistHelper

HistHelper::HistHelper()
  : N_HIST(0), n_nonempty_bins(0)
  , n_vx(0), n_vy(0), n_vz(0)
  , nh_x(0), nh_y(0), nh_z(0)
{}

HistHelper HistHelper::merge(const int direction, const HistHelper& a, const HistHelper& b)
{
    HistHelper c;
    c.N_HIST = a.N_HIST + b.N_HIST;
    c.n_nonempty_bins = a.n_nonempty_bins + b.n_nonempty_bins;
    if (0 == direction) // x
    {
        c.n_vx = a.n_vx + b.n_vx;
        assert(a.n_vy == b.n_vy); c.n_vy = a.n_vy;
        assert(a.n_vz == b.n_vz); c.n_vz = a.n_vz;
        c.nh_x = a.nh_x + b.nh_x;
        assert(a.nh_y == b.nh_y); c.nh_y = a.nh_y;
        assert(a.nh_z == b.nh_z); c.nh_z = a.nh_z;

    } else if (1 == direction) // y
    {
        assert(a.n_vx == b.n_vx); c.n_vx = a.n_vx;
        c.n_vy = a.n_vy + b.n_vy;
        assert(a.n_vz == b.n_vz); c.n_vz = a.n_vz;
        assert(a.nh_x == b.nh_x); c.nh_x = a.nh_x;
        c.nh_y = a.nh_y + b.nh_y;
        assert(a.nh_z == b.nh_z); c.nh_z = a.nh_z;

    } else if (2 == direction) // z
    {
        assert(a.n_vx == b.n_vx); c.n_vx = a.n_vx;
        assert(a.n_vy == b.n_vy); c.n_vy = a.n_vy;
        c.n_vz = a.n_vz + b.n_vz;
        assert(a.nh_x == b.nh_x); c.nh_x = a.nh_x;
        assert(a.nh_y == b.nh_y); c.nh_y = a.nh_y;
        c.nh_z = a.nh_z + b.nh_z;
    }
    return c;
}

std::istream& operator>>(std::istream& in, HistHelper& helper)
{
    std::string line;
    std::getline(in, line);
    in >> helper.N_HIST >> helper.n_nonempty_bins >> helper.n_vx >> helper.n_vy
            >> helper.n_vz >> helper.nh_x >> helper.nh_y >> helper.nh_z;
    return in;
}

std::ostream& operator<<(std::ostream& out, const HistHelper& helper)
{
    out << "N_HIST=" << helper.N_HIST
        << " n_nonempty_bins=" << helper.n_nonempty_bins
        << " n_vx=" << helper.n_vx
        << " n_vy=" << helper.n_vy
        << " n_vz=" << helper.n_vz << std::endl
        << " nh_x=" << helper.nh_x
        << " nh_y=" << helper.nh_y
        << " nh_z=" << helper.nh_z << std::endl;
    return out;
}


/////////////////////////////////////////////////////////////////////////////////////
// HistRect


HistRect::HistRect(int nHistX, int nHistY,
        std::vector<std::shared_ptr<const Hist>> hists)
  : _nHistX(nHistX), _nHistY(nHistY), _hists(hists)
{
    /// TODO:
}


/////////////////////////////////////////////////////////////////////////////////////
// HistDomain


HistDomain::HistDomain(
        HistHelper helper, std::vector<std::shared_ptr<Hist> > hists)
  : HistGrid(helper, hists)
{

}

HistDomain::HistDomain(const std::string& dir, const std::string& name, int iProc,
        const std::vector<std::string> &vars)
{
    char iProcStr[6];
    sprintf(iProcStr, "%05d", iProc);
    if (isFileExist(dir + "/pdfs-" + name + "." + iProcStr)) {
        HistDomainReaderPacked reader(dir, name, iProcStr, vars);
        reader.read(m_helper, m_hists);
    } else if (isFileExist(dir + "/pdfhelper." + iProcStr)) {
        HistDomainReaderManyFiles reader(dir, name, iProcStr, vars);
        reader.read(m_helper, m_hists);
    }
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistVolume

HistVolume::HistVolume(const std::string& dir, const std::string& name,
        const std::vector<int>& dim, const std::vector<std::string> &vars)
  : m_dimDomains(dim)
  , m_dir(dir)
  , m_name(name)
  , m_vars(vars)
  , m_helperCached(false)
{
    // QElapsedTimer timer;
    // timer.start();

    m_domains.resize(nDomains());
    if (isFileExist(dir + "/pdfs-ycolumn-001.00000")) {
        int nYColumns = dim[0] * dim[2];
        for (int iYColumn = 0; iYColumn < nYColumns; ++iYColumn) {
            char iYColumnStr[6];
            sprintf(iYColumnStr, "%05d", iYColumn);
            auto histDomains =
                    HistYColumnReader(dir, name, iYColumnStr, vars).read();
            int nYDomains = dim[1];
            for (int iYDomain = 0; iYDomain < nYDomains; ++iYDomain) {
                auto yColumnIds = Extent(dim[0], dim[2]).flattoids(iYColumn);
                int iDomain =
                        Extent(dim).idstoflat(
                            yColumnIds[0], iYDomain, yColumnIds[1]);
                m_domains[iDomain] = histDomains[iYDomain];
            }
        }
    } else {
        for (int iDomain = 0; iDomain < nDomains(); ++iDomain) {
            if (iDomain % 10000 == 0)
                std::cout << "constructing Domain " << iDomain << "/"
                        << nDomains() << std::endl;
            m_domains[iDomain] =
                    std::make_shared<HistDomain>(dir, name, iDomain, vars);
        }
    }

    // auto elapsed = timer.elapsed();
    // auto vh = helper();

    // qDebug() << "numHists:" << vh.N_HIST << "loadHistVolume:" << elapsed;

}

HistHelper HistVolume::helper() const
{
    if (m_helperCached)
        return m_helper;
    // gather helpers
    std::vector<HistHelper> oldHelpers(m_domains.size());
    for (unsigned int iDomain = 0; iDomain < m_domains.size(); ++iDomain)
        oldHelpers[iDomain] = m_domains[iDomain]->helper();
    auto oldDim = m_dimDomains;
    // collapse per each dimension
    for (unsigned int iDim = 0; iDim < m_dimDomains.size(); ++iDim)
    {
        // create container for the collapsed helpers
        std::vector<int> newDim(oldDim.begin() + 1, oldDim.end());
        assert(newDim.size() == oldDim.size() - 1);
        int newNDomains = [&newDim]() {
            int prod = 1;
            for (auto dim : newDim)
                prod *= dim;
            return prod;
        }();
        std::vector<HistHelper> newHelpers(newNDomains);
        // collapse
        for (int iDomain = 0; iDomain < newNDomains; ++iDomain)
        {
            newHelpers[iDomain] = oldHelpers[0 + iDomain * oldDim[0]];
            for (int i = 1; i < oldDim[0]; ++i)
                newHelpers[iDomain] = HistHelper::merge(iDim,
                    newHelpers[iDomain],
                    oldHelpers[i + iDomain * oldDim[0]]);
        }
        // replace the old helpers and recursive
        oldDim = newDim;
        oldHelpers = std::move(newHelpers);
    }
    assert(oldHelpers.size() == 1);
    m_helperCached = true;
    m_helper = oldHelpers[0];
    return m_helper;
}

std::shared_ptr<Hist> HistVolume::hist(int flatId)
{
    auto dimHists = helper().dimHists();
    auto histIds = dimHists.flattoids(flatId);
    std::vector<int> domainIds(histIds.size());
    std::vector<int> dHistIds(histIds.size());
    assert(dimHists.nDim() == int(m_dimDomains.size()));
    for (unsigned int i = 0; i < domainIds.size(); ++i)
    {
        int nHistPerDomainPerDim = dimHists[i] / m_dimDomains[i];
        domainIds[i] = histIds[i] / nHistPerDomainPerDim;
        dHistIds[i]  = histIds[i] % nHistPerDomainPerDim;
    }
    return domain(domainIds)->hist(dHistIds);
}

std::shared_ptr<const Hist> HistVolume::hist(int flatId) const
{
    auto dimHists = helper().dimHists();
    auto histIds = dimHists.flattoids(flatId);
    std::vector<int> domainIds(histIds.size());
    std::vector<int> dHistIds(histIds.size());
    assert(dimHists.nDim() == int(m_dimDomains.size()));
    for (unsigned int i = 0; i < domainIds.size(); ++i)
    {
        int nHistPerDomainPerDim = dimHists[i] / m_dimDomains[i];
        domainIds[i] = histIds[i] / nHistPerDomainPerDim;
        dHistIds[i]  = histIds[i] % nHistPerDomainPerDim;
    }
    return domain(domainIds)->hist(dHistIds);
}

int HistVolume::nDomains() const
{
    int prod = 1;
    for (auto nDomain : m_dimDomains)
        prod *= nDomain;
    return prod;
}

std::shared_ptr<HistRect> HistVolume::xySlice(int z) const
{
    auto nHist = helper().nh_x * helper().nh_y;
    std::vector<std::shared_ptr<const Hist>> hists(nHist);
    for (auto x = 0; x < helper().nh_x; ++x)
    for (auto y = 0; y < helper().nh_y; ++y) {
        hists[x + helper().nh_x * y] = hist(x, y, z);
    }
    return std::make_shared<HistRect>(helper().nh_x, helper().nh_y, hists);
}

std::shared_ptr<HistRect> HistVolume::xzSlice(int y) const
{
    auto nHist = helper().nh_x * helper().nh_z;
    std::vector<std::shared_ptr<const Hist>> hists(nHist);
    for (auto x = 0; x < helper().nh_x; ++x)
    for (auto z = 0; z < helper().nh_z; ++z) {
        hists[x + helper().nh_x * z] = hist(x, y, z);
    }
    return std::make_shared<HistRect>(helper().nh_x, helper().nh_z, hists);
}

std::shared_ptr<HistRect> HistVolume::yzSlice(int x) const
{
    auto nHist = helper().nh_y * helper().nh_z;
    std::vector<std::shared_ptr<const Hist>> hists(nHist);
    for (auto y = 0; y < helper().nh_y; ++y)
    for (auto z = 0; z < helper().nh_z; ++z) {
        hists[y + helper().nh_y * z] = hist(x, y, z);
    }
    return std::make_shared<HistRect>(helper().nh_y, helper().nh_z, hists);
}

std::vector<int> HistVolume::dhtoids(const std::vector<int> &dIds, const std::vector<int> &hIds) const
{
    assert(dIds.size() == 3);
    assert(dIds.size() == hIds.size());
    auto helper = domain(0)->helper();
    int nLocalHists[3] = { helper.nh_x, helper.nh_y, helper.nh_z };
    std::vector<int> ids;
    ids.resize(dIds.size());
    for (unsigned int i = 0; i < dIds.size(); ++i)
        ids[i] = dIds[i] * nLocalHists[i] + hIds[i];
    return ids;
}

std::vector<int> HistVolume::dhtoids(int dId, int hId) const
{
    int x, y, z;
    this->dhtoids(dId, hId, &x, &y, &z);
    return { x, y, z };
}

void HistVolume::dhtoids(int dId, int hId, int* x, int* y, int* z) const
{
    int dIds[3], hIds[3];
    Extent(dimDomains()).flattoids(dId, &dIds[0], &dIds[1], &dIds[2]);
    auto helper = domain(0)->helper();
    helper.dimHists().flattoids(hId, &hIds[0], &hIds[1], &hIds[2]);
    int nLocalHists[3] = { helper.nh_x, helper.nh_y, helper.nh_z };
    *x = dIds[0] * nLocalHists[0] + hIds[0];
    *y = dIds[1] * nLocalHists[1] + hIds[1];
    *z = dIds[2] * nLocalHists[2] + hIds[2];
}

int HistVolume::dhtoflat(const std::vector<int> &dIds, const std::vector<int> &hIds) const
{
    return this->helper().dimHists().idstoflat(this->dhtoids(dIds, hIds));
}

int HistVolume::dhtoflat(int dId, int hId) const
{
    int x, y, z;
    this->dhtoids(dId, hId, &x, &y, &z);
    return this->helper().dimHists().idstoflat(x, y, z);
}
