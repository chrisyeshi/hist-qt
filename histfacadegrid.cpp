#include "histfacadegrid.h"
#include <fstream>
#include <data/histreader.h>
#include <QElapsedTimer>

namespace {
bool isFileExist(const std::string &path)
{
    std::ifstream f(path.c_str());
    return f.good();
}
} // namespace

/**
 * @brief HistFacadeDomain::HistFacadeDomain
 * @param dir
 * @param name
 * @param iDomain
 * @param vars
 */
HistFacadeDomain::HistFacadeDomain(
        const std::string &dir, const std::string &name, int iDomain,
        const std::vector<std::string> &vars) {
    char iProcStr[6];
    sprintf(iProcStr, "%05d", iDomain);
    std::vector<std::shared_ptr<Hist>> hists;
    if (isFileExist(dir + "/pdfs-" + name + "." + iProcStr)) {
        HistDomainReaderPacked reader(dir, name, iProcStr, vars);
        reader.read(_helper, hists);
    } else if (isFileExist(dir + "/pdfhelper." + iProcStr)) {
        HistDomainReaderManyFiles reader(dir, name, iProcStr, vars);
        reader.read(_helper, hists);
    }
    _hists.resize(hists.size());
    for (decltype(hists.size()) iHist = 0; iHist < hists.size(); ++iHist) {
        _hists[iHist] = HistFacade::create(hists[iHist], vars);
    }
}

/**
 * @brief HistFacadeVolume::HistFacadeVolume
 * @param dir
 * @param name
 * @param dims
 * @param vars
 */
HistFacadeVolume::HistFacadeVolume(
        const std::string &dir, const std::string &name,
        const std::vector<int> &dims, const std::vector<std::string> &vars)
  : _dimDomains(dims), _dir(dir), _name(name), _vars(vars), _helperCached(false)
{
    QElapsedTimer timer;
    timer.start();

    _domains.resize(nDomains());
    if (isFileExist(dir + "/pdfs-ycolumn-001.00000")) {
        int nYColumns = dims[0] * dims[2];
        for (int iYColumn = 0; iYColumn < nYColumns; ++iYColumn) {
            char iYColumnStr[6];
            sprintf(iYColumnStr, "%05d", iYColumn);
            auto histDomains =
                    HistFacadeYColumnReader(
                        dir, name, iYColumnStr, vars).read();
            int nYDomains = dims[1];
            for (int iYDomain = 0; iYDomain < nYDomains; ++iYDomain) {
                auto yColumnIds = Extent(dims[0], dims[2]).flattoids(iYColumn);
                int iDomain =
                        Extent(dims).idstoflat(
                            yColumnIds[0], iYDomain, yColumnIds[1]);
                _domains[iDomain] = histDomains[iYDomain];
            }
        }
    } else {
        for (int iDomain = 0; iDomain < nDomains(); ++iDomain)
        {
            if (iDomain % 10000 == 0)
                std::cout << "constructing Domain " << iDomain << "/"
                        << nDomains() << std::endl;
            _domains[iDomain] =
                    std::make_shared<HistFacadeDomain>(
                        dir, name, iDomain, vars);
        }
    }
}

HistHelper HistFacadeVolume::helper() const {
    if (_helperCached)
        return _helper;
    // gather helpers
    std::vector<HistHelper> oldHelpers(_domains.size());
    for (unsigned int iDomain = 0; iDomain < _domains.size(); ++iDomain)
        oldHelpers[iDomain] = _domains[iDomain]->helper();
    auto oldDim = _dimDomains;
    // collapse per each dimension
    for (int iDim = 0; iDim < _dimDomains.nDim(); ++iDim)
    {
        // create container for the collapsed helpers
        std::vector<int> newDim(oldDim.begin() + 1, oldDim.end());
        assert(int(newDim.size()) == oldDim.nDim() - 1);
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
    _helperCached = true;
    _helper = oldHelpers[0];
    return _helper;
}

std::shared_ptr<HistFacade> HistFacadeVolume::hist(int flatId)
{
    auto dimHists = helper().dimHists();
    auto histIds = dimHists.flattoids(flatId);
    std::vector<int> domainIds(histIds.size());
    std::vector<int> dHistIds(histIds.size());
    assert(dimHists.nDim() == int(_dimDomains.nDim()));
    for (unsigned int i = 0; i < domainIds.size(); ++i)
    {
        int nHistPerDomainPerDim = dimHists[i] / _dimDomains[i];
        domainIds[i] = histIds[i] / nHistPerDomainPerDim;
        dHistIds[i]  = histIds[i] % nHistPerDomainPerDim;
    }
    return domain(domainIds)->hist(dHistIds);
}

std::shared_ptr<const HistFacade> HistFacadeVolume::hist(int flatId) const
{
    auto dimHists = helper().dimHists();
    auto histIds = dimHists.flattoids(flatId);
    std::vector<int> domainIds(histIds.size());
    std::vector<int> dHistIds(histIds.size());
    assert(dimHists.nDim() == _dimDomains.nDim());
    for (unsigned int i = 0; i < domainIds.size(); ++i)
    {
        int nHistPerDomainPerDim = dimHists[i] / _dimDomains[i];
        domainIds[i] = histIds[i] / nHistPerDomainPerDim;
        dHistIds[i]  = histIds[i] % nHistPerDomainPerDim;
    }
    return domain(domainIds)->hist(dHistIds);
}

std::shared_ptr<HistFacadeDomain> HistFacadeVolume::domain(int flatId) {
    return _domains[flatId];
}

std::shared_ptr<const HistFacadeDomain> HistFacadeVolume::domain(
        int flatId) const {
    return _domains[flatId];
}

std::shared_ptr<HistFacadeDomain> HistFacadeVolume::domain(
        const std::vector<int> &ids) {
    return domain(_dimDomains.idstoflat(ids));
}

std::shared_ptr<const HistFacadeDomain> HistFacadeVolume::domain(
        const std::vector<int> &ids) const {
    return domain(_dimDomains.idstoflat(ids));
}

int HistFacadeVolume::nDomains() const
{
    int prod = 1;
    for (auto nDomain : _dimDomains) {
        prod *= nDomain;
    }
    return prod;
}

std::shared_ptr<HistFacadeRect> HistFacadeVolume::xySlice(int z) const
{
    auto nHist = helper().nh_x * helper().nh_y;
    std::vector<std::shared_ptr<const HistFacade>> hists(nHist);
    for (auto x = 0; x < helper().nh_x; ++x)
    for (auto y = 0; y < helper().nh_y; ++y) {
        hists[x + helper().nh_x * y] = hist(x, y, z);
    }
    return std::make_shared<HistFacadeRect>(
            helper().nh_x, helper().nh_y, hists);
}

std::shared_ptr<HistFacadeRect> HistFacadeVolume::xzSlice(int y) const
{
    auto nHist = helper().nh_x * helper().nh_z;
    std::vector<std::shared_ptr<const HistFacade>> hists(nHist);
    for (auto x = 0; x < helper().nh_x; ++x)
    for (auto z = 0; z < helper().nh_z; ++z) {
        hists[x + helper().nh_x * z] = hist(x, y, z);
    }
    return std::make_shared<HistFacadeRect>(
            helper().nh_x, helper().nh_z, hists);
}

std::shared_ptr<HistFacadeRect> HistFacadeVolume::yzSlice(int x) const
{
    auto nHist = helper().nh_y * helper().nh_z;
    std::vector<std::shared_ptr<const HistFacade>> hists(nHist);
    for (auto y = 0; y < helper().nh_y; ++y)
    for (auto z = 0; z < helper().nh_z; ++z) {
        hists[y + helper().nh_y * z] = hist(x, y, z);
    }
    return std::make_shared<HistFacadeRect>(
                helper().nh_y, helper().nh_z, hists);
}

std::vector<int> HistFacadeVolume::dhtoids(
        const std::vector<int> &dIds, const std::vector<int> &hIds) const
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

std::vector<int> HistFacadeVolume::dhtoids(int dId, int hId) const
{
    int x, y, z;
    this->dhtoids(dId, hId, &x, &y, &z);
    return { x, y, z };
}

void HistFacadeVolume::dhtoids(int dId, int hId, int *x, int *y, int *z) const
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

int HistFacadeVolume::dhtoflat(
        const std::vector<int> &dIds, const std::vector<int> &hIds) const
{
    return this->helper().dimHists().idstoflat(this->dhtoids(dIds, hIds));
}

int HistFacadeVolume::dhtoflat(int dId, int hId) const
{
    int x, y, z;
    this->dhtoids(dId, hId, &x, &y, &z);
    return this->helper().dimHists().idstoflat(x, y, z);
}

