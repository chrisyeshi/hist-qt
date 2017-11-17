#ifndef MULTIBLOCKTOPOLOGY_H
#define MULTIBLOCKTOPOLOGY_H

#include <vector>
#include <vec.h>
#include <functional.h>
#include <data/Extent.h>
//#include <extent.h>

/**
 * class BoundingBox
 */
template <typename Type>
class BoundingBox {
public:
    BoundingBox() = default;
    BoundingBox(yy::tvec<Type, 3> lowerCorner, yy::tvec<Type, 3> upperCorner)
          : _lowerCorner(lowerCorner), _upperCorner(upperCorner) {}

public:
    yy::tvec<Type, 3> lower() const { return _lowerCorner; }
    yy::tvec<Type, 3> upper() const { return _upperCorner; }

private:
    yy::tvec<Type, 3> _lowerCorner, _upperCorner;
};

/**
 * class BlockSpec
 */
class BlockSpec {
public:
    BlockSpec() = default;
    BlockSpec(Extent nDomains, Extent nGridPts, yy::ivec3 lowerCorner)
          : _nDomains(nDomains), _nGridPts(nGridPts),
            _lowerCorner(lowerCorner) {
        assert(nGridPts[0] % nDomains[0] == 0);
        assert(nGridPts[1] % nDomains[1] == 0);
        assert(nGridPts[2] % nDomains[2] == 0);
    }
    BlockSpec(Extent nDomains, Extent nGridPts, yy::ivec3 lowerCorner,
            BoundingBox<float> physicalBoundingBox)
          : _nDomains(nDomains), _nGridPts(nGridPts), _lowerCorner(lowerCorner),
            _physicalBoundingBox(physicalBoundingBox) {
        assert(nGridPts[0] % nDomains[0] == 0);
        assert(nGridPts[1] % nDomains[1] == 0);
        assert(nGridPts[2] % nDomains[2] == 0);
    }

public:
    Extent nDomains() const { return _nDomains; }
    int totalDomainCount() const { return _nDomains.nElement(); }
    int totalGridPtCount() const { return _nGridPts.nElement(); }
    const Extent& nGridPts() const { return _nGridPts; }
    yy::ivec3 lowerCorner() const { return _lowerCorner; }
    BoundingBox<float> physicalBoundingBox() const {
        return _physicalBoundingBox;
    }

private:
    Extent _nDomains;
    Extent _nGridPts;
    yy::ivec3 _lowerCorner;
    BoundingBox<float> _physicalBoundingBox;
};

/**
 * class Topology
 */
class MultiBlockTopology {
public:
    MultiBlockTopology() = default;
    template <typename T>
    MultiBlockTopology(T blockSpecs) : _blockSpecs(blockSpecs) {}

public:
    int blockId(int rank) const {
        int offset = 0;
        for (unsigned int iBlock = 0; iBlock < _blockSpecs.size(); ++iBlock) {
            const auto& blockSpec = _blockSpecs[iBlock];
            if (offset <= rank &&
                    rank < offset + blockSpec.totalDomainCount()) {
                return iBlock;
            }
            offset += blockSpec.totalDomainCount();
        }
        assert(false);
        return -1;
    }
    int blockDomainRank(int rank) const {
        int offset = 0;
        for (unsigned int iBlock = 0; iBlock < _blockSpecs.size(); ++iBlock) {
            const auto& blockSpec = _blockSpecs[iBlock];
            if (offset <= rank &&
                    rank < offset + blockSpec.totalDomainCount()) {
                return rank - offset;
            }
            offset += blockSpec.totalDomainCount();
        }
        assert(false);
        return -1;
    }
    int blockGridPtCount(int iBlock) const {
        return _blockSpecs[iBlock].totalGridPtCount();
    }
    yy::ivec3 blockDimensions(int iBlock) const {
        auto nGridPts = _blockSpecs[iBlock].nGridPts();
        return yy::ivec3(nGridPts[0], nGridPts[1], nGridPts[2]);
    }
    yy::ivec3 blockDomainDimensions(int rank) const {
        return std::vector<int>(_blockSpecs[blockId(rank)].nDomains());
    }
    int blockDomainCount(int rank) const {
        return _blockSpecs[blockId(rank)].nDomains().nElement();
    }
    yy::ivec3 domainDimensions(int rank) const {
        yy::ivec3 nGridPts = blockDimensions(blockId(rank));
        yy::ivec3 nDomains = blockDomainDimensions(rank);
        return nGridPts / nDomains;
    }
    yy::ivec3 blockDomainOffsets(int rank) const {
        yy::ivec3 domainIds = blockDomainIds(rank);
        return domainDimensions(rank) * domainIds;
    }
    yy::ivec3 blockDomainIds(int rank) const {
        return Extent(blockDomainDimensions(rank))
                .flattoids(blockDomainRank(rank));
    }
    int blockCount() const { return _blockSpecs.size(); }
    BlockSpec blockSpec(int iBlock) const { return _blockSpecs[iBlock]; }

private:
    std::vector<BlockSpec> _blockSpecs;
};

/**
 * @brief The UniformGridTopology class
 */
class UniformGridTopology {
public:
    UniformGridTopology() = default;
    UniformGridTopology(yy::ivec3 dimVoxels, yy::ivec3 dimProcs,
            yy::ivec3 dimHistsPerDomain,
            BoundingBox<float> physicalBoundingBox)
          : _dimVoxels(dimVoxels), _dimProcs(dimProcs),
            _dimHistsPerDomain(dimHistsPerDomain),
            _physicalBoundingBox(physicalBoundingBox) {}

public:
    yy::ivec3 dimVoxels() const { return _dimVoxels; }
    yy::ivec3 dimProcs() const { return _dimProcs; }
    yy::ivec3 dimHistsPerDomain() const { return _dimHistsPerDomain; }
    BoundingBox<float> physicalBoundingBox() const {
        return _physicalBoundingBox;
    }

private:
    yy::ivec3 _dimVoxels;
    yy::ivec3 _dimProcs;
    yy::ivec3 _dimHistsPerDomain;
    BoundingBox<float> _physicalBoundingBox;
};

#endif // MULTIBLOCKTOPOLOGY_H
