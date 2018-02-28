#include "histfacadegrid.h"
#include <cmath>
#include <fstream>
#include <data/histreader.h>
#include <QElapsedTimer>
#include <util.h>
#include <data/directory.h>

namespace {

bool isFileExist(const std::string &path)
{
    std::ifstream f(path.c_str());
    return f.good();
}

std::shared_ptr<HistFacadeDomain> getNullHistDomain(
        const yy::ivec3& nVoxels, const yy::ivec3& nHists) {
    HistHelper defaultHelper;
    defaultHelper.n_vx = nVoxels.x();
    defaultHelper.n_vy = nVoxels.y();
    defaultHelper.n_vz = nVoxels.z();
    defaultHelper.nh_x = nHists.x();
    defaultHelper.nh_y = nHists.y();
    defaultHelper.nh_z = nHists.z();
    defaultHelper.N_HIST =
            defaultHelper.nh_x * defaultHelper.nh_y * defaultHelper.nh_z;
    auto defaultHistDomain =
            std::make_shared<HistFacadeDomain>(
                defaultHelper,
                std::vector<std::shared_ptr<HistFacade>>(
                    defaultHelper.N_HIST, std::make_shared<HistNullFacade>()));
    return defaultHistDomain;
}

yy::ivec3 getMultiBlockDomainCounts(const MultiBlockTopology& topo) {
    if (4 == topo.blockCount()) {
        int x = topo.blockSpec(0).nDomains()[0]
                + topo.blockSpec(1).nDomains()[0]
                + topo.blockSpec(2).nDomains()[0];
        int y = topo.blockSpec(3).nDomains()[1]
                + topo.blockSpec(0).nDomains()[1];
        int z = topo.blockSpec(0).nDomains()[2];
        return yy::ivec3(x, y, z);
    }
    assert(false);
    return yy::ivec3(-1, -1, -1);
}

yy::ivec3 getMultiBlockDomainIdOffsets(
        const MultiBlockTopology& topo, int iBlock) {
    if (4 == topo.blockCount()) {
        if (0 == iBlock) {
            return yy::ivec3(0, topo.blockSpec(3).nDomains()[1], 0);
        } else if (1 == iBlock) {
            return yy::ivec3(topo.blockSpec(0).nDomains()[0],
                    topo.blockSpec(3).nDomains()[1], 0);
        } else if (2 == iBlock) {
            return yy::ivec3(
                    topo.blockSpec(0).nDomains()[0]
                        + topo.blockSpec(1).nDomains()[0],
                    topo.blockSpec(3).nDomains()[1], 0);
        } else if (3 == iBlock) {
            return yy::ivec3(topo.blockSpec(0).nDomains()[0], 0, 0);
        }
    }
    assert(false);
    return yy::ivec3(-1, -1, -1);
}

yy::ivec3 getMultiBlockDomainVoxelCounts(
        const MultiBlockTopology& topo, const yy::ivec3& domainIds) {
    if (4 == topo.blockCount()) {
        int xDomain1 = topo.blockSpec(0).nDomains()[0];
        int xGridPt1 = topo.blockSpec(0).nGridPts()[0];
        int xDomain2 = topo.blockSpec(1).nDomains()[0];
        int xGridPt2 = topo.blockSpec(1).nGridPts()[0];
        int xDomain3 = topo.blockSpec(2).nDomains()[0];
        int xGridPt3 = topo.blockSpec(2).nGridPts()[0];
        int yDomain1 = topo.blockSpec(3).nDomains()[1];
        int yGridPt1 = topo.blockSpec(3).nGridPts()[1];
        int yDomain2 = topo.blockSpec(0).nDomains()[1];
        int yGridPt2 = topo.blockSpec(0).nGridPts()[1];
        int zDomain = topo.blockSpec(0).nDomains()[2];
        int zGridPt = topo.blockSpec(0).nGridPts()[2];
        int x, y, z;
        // x
        if (domainIds.x() < xDomain1) {
            x = xGridPt1 / xDomain1;
        } else if (domainIds.x() < xDomain1 + xDomain2) {
            x = xGridPt2 / xDomain2;
        } else if (domainIds.x() < xDomain1 + xDomain2 + xDomain3) {
            x = xGridPt3 / xDomain3;
        } else {
            assert(false);
        }
        // y
        if (domainIds.y() < yDomain1) {
            y = yGridPt1 / yDomain1;
        } else if (domainIds.y() < yDomain1 + yDomain2) {
            y = yGridPt2 / yDomain2;
        } else {
            assert(false);
        }
        // z
        z = zGridPt / zDomain;
        return yy::ivec3(x, y, z);
    }
    assert(false);
    return yy::ivec3(-1, -1, -1);
}

yy::ivec3 getMultiBlockDomainHistCounts(
        const MultiBlockTopology& topo, const yy::ivec3& domainIds) {
    return yy::ivec3(1, 1, 1);
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
        std::vector<int> dims, const std::vector<std::string> &vars)
  : _dimDomains(dims), _dir(dir), _name(name), _vars(vars), _helperCached(false)
{
    _domains.resize(nDomains());
    if (isFileExist(dir + "/pdfs-ycolumn-001.00000")) {
        auto entries = entryNamesInDirectory(dir);
        auto yColumns =
                yy::fp::filter(entries, [name](const std::string& entry) {
            return entry.size() >= 16 && name == entry.substr(13, 3);
        });
        if (yColumns.size() != dims[0] * dims[2]) {
            // everything in one file, other than "." and ".."
            assert(3 == entries.size());
            _domains = HistFacadeYColumnReader(dir, name, "00000", vars).read();
        } else {
            // actual y columns
            int nYColumns = dims[0] * dims[2];
            for (int iYColumn = 0; iYColumn < nYColumns; ++iYColumn) {
                char iYColumnStr[6];
                sprintf(iYColumnStr, "%05d", iYColumn);
                auto histDomains =
                        HistFacadeYColumnReader(
                            dir, name, iYColumnStr, vars).read();
                int nYDomains = dims[1];
                for (int iYDomain = 0; iYDomain < nYDomains; ++iYDomain) {
                    auto yColumnIds =
                            Extent(dims[0], dims[2]).flattoids(iYColumn);
                    int iDomain =
                            Extent(dims).idstoflat(
                                yColumnIds[0], iYDomain, yColumnIds[1]);
                    _domains[iDomain] = histDomains[iYDomain];
                }
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

HistFacadeVolume::HistFacadeVolume(std::string dir, std::string name,
        const MultiBlockTopology& topo, std::vector<std::string> vars)
      : _dir(dir), _name(name), _vars(vars), _helperCached(false) {
    _dimDomains = getMultiBlockDomainCounts(topo);
    _domains.resize(nDomains());
    for (auto z = 0; z < _dimDomains[2]; ++z)
    for (auto y = 0; y < _dimDomains[1]; ++y)
    for (auto x = 0; x < _dimDomains[0]; ++x) {
        _domains[_dimDomains.idstoflat(x, y, z)] =
                getNullHistDomain(
                    getMultiBlockDomainVoxelCounts(topo, yy::ivec3(x, y, z)),
                    getMultiBlockDomainHistCounts(topo, yy::ivec3(x, y, z)));
    }
    for (auto iBlock = 0; iBlock < topo.blockCount(); ++iBlock) {
        auto iBlockStr = yy::sprintf("%05d", iBlock);
        auto histDomains =
                HistFacadeYColumnReader(dir, name, iBlockStr, vars).read();
        yy::ivec3 domainIdOffsets = getMultiBlockDomainIdOffsets(topo, iBlock);
        Extent blockDomainExtent = topo.blockSpec(iBlock).nDomains();
        for (auto iBlockDomain = 0; iBlockDomain < histDomains.size();
                ++iBlockDomain) {
            yy::ivec3 blockDomainIds =
                    blockDomainExtent.flattoids(iBlockDomain);
            yy::ivec3 domainIds = domainIdOffsets + blockDomainIds;
            int domainFlatId = _dimDomains.idstoflat(domainIds);
            _domains[domainFlatId] = std::move(histDomains[iBlockDomain]);
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

int HistFacadeVolume::nDomains() const {
    return _dimDomains.nElement();
}

HistFacadeVolume::Stats HistFacadeVolume::stats() const {
    if (_statsCached)
        return _stats;
    std::vector<float> sums(_vars.size(), 0.f);
    std::vector<float> mins(_vars.size(), std::numeric_limits<float>::max());
    std::vector<float> maxs(_vars.size(), std::numeric_limits<float>::lowest());
    int nonEmptyHistCount = 0;
    for (int iHist = 0; iHist < nHist(); ++iHist) {
        auto histAverages = hist(iHist)->hist()->means();
        if (histAverages.empty()) continue;
        ++nonEmptyHistCount;
        for (int iVar = 0; iVar < _vars.size(); ++iVar) {
            auto average = histAverages[iVar];
            sums[iVar] += average;
            mins[iVar] = std::min(average, mins[iVar]);
            maxs[iVar] = std::max(average, maxs[iVar]);
        }
    }
    std::vector<float> averages = yy::fp::map(sums, [&](float sum) {
        return sum / nonEmptyHistCount;
    });
    Stats stats;
    for (int iVar = 0; iVar < _vars.size(); ++iVar) {
        stats.means[_vars[iVar]] = averages[iVar];
        stats.meanRanges[_vars[iVar]] = {mins[iVar], maxs[iVar]};
    }
    _stats = stats;
    _statsCached = true;
    return stats;
}

std::shared_ptr<HistFacadeRect> HistFacadeVolume::xySlice(int z) const {
    try {
        if (0 < _cachedXYSlices.count(z)) {
            return _cachedXYSlices.at(z);
        }
    } catch (...) {
        std::cout << "HistFacadeVolume::xySlice" << std::endl;
    }
    auto nHist = helper().nh_x * helper().nh_y;
    std::vector<std::shared_ptr<const HistFacade>> hists(nHist);
    for (auto x = 0; x < helper().nh_x; ++x)
    for (auto y = 0; y < helper().nh_y; ++y) {
        hists[x + helper().nh_x * y] = hist(x, y, z);
    }
    auto slice =
            std::make_shared<HistFacadeRect>(
                helper().nh_x, helper().nh_y, hists);
    _cachedXYSlices[z] = slice;
    return slice;
}

std::shared_ptr<HistFacadeRect> HistFacadeVolume::xzSlice(int y) const {
    try {
        if (0 < _cachedXZSlices.count(y)) {
            return _cachedXZSlices.at(y);
        }
    } catch (...) {
        std::cout << "HistFacadeVolume::xzSlice" << std::endl;
    }
    auto nHist = helper().nh_x * helper().nh_z;
    std::vector<std::shared_ptr<const HistFacade>> hists(nHist);
    for (auto x = 0; x < helper().nh_x; ++x)
    for (auto z = 0; z < helper().nh_z; ++z) {
        hists[x + helper().nh_x * z] = hist(x, y, z);
    }
    auto slice =
            std::make_shared<HistFacadeRect>(
                helper().nh_x, helper().nh_z, hists);
    _cachedXZSlices[y] = slice;
    return slice;
}

std::shared_ptr<HistFacadeRect> HistFacadeVolume::yzSlice(int x) const {
    try {
        if (0 < _cachedYZSlices.count(x)) {
            return _cachedYZSlices.at(x);
        }
    } catch (...) {
        std::cout << "HistFacadeVolume::yzSlice" << std::endl;
    }
    auto nHist = helper().nh_y * helper().nh_z;
    std::vector<std::shared_ptr<const HistFacade>> hists(nHist);
    for (auto y = 0; y < helper().nh_y; ++y)
    for (auto z = 0; z < helper().nh_z; ++z) {
        hists[y + helper().nh_y * z] = hist(x, y, z);
    }
    auto slice =
            std::make_shared<HistFacadeRect>(
                helper().nh_y, helper().nh_z, hists);
    _cachedYZSlices[x] = slice;
    return slice;
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

