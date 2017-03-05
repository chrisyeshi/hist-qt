#ifndef MASK_H
#define MASK_H

#include <data/Extent.h>

/**
 * @brief The BoolMask2D class
 */
class BoolMask2D {
public:
    BoolMask2D() = default;
    BoolMask2D(Extent dims, std::vector<bool> mask)
      : _dims(dims), _mask(mask) {}
    bool isSelected(int flatId) const {
        return _mask[flatId];
    }

private:
    Extent _dims;
    std::vector<bool> _mask;
};

/**
 * @brief The BoolMask3D class
 */
class BoolMask3D {
public:
    void reset(Extent dims, bool flag) {
        _dims = dims;
        _mask.resize(_dims[0] * _dims[1] * _dims[2]);
        for (auto value : _mask)
            value = flag;
    }
    void resetToAllTrue(Extent dims) {
        reset(dims, true);
    }
    void resetToAllTrue() {
        resetToAllTrue(_dims);
    }
    void setSelected(int flatId, bool isSelected) {
        _mask[flatId] = isSelected;
    }
    bool isSelected(int flatId) const {
        return _mask[flatId];
    }
    bool isSelected(int x, int y, int z) {
        return _mask[x + _dims[0] * y + _dims[0] * _dims[1] * z];
    }
    BoolMask2D xySlice(int z) {
        auto nHist = _dims[0] * _dims[1];
        std::vector<bool> mask(nHist);
        for (auto x = 0; x < _dims[0]; ++x)
        for (auto y = 0; y < _dims[1]; ++y) {
            mask[x + _dims[0] * y] = isSelected(x, y, z);
        }
        return BoolMask2D(Extent(_dims[0], _dims[1]), mask);
    }
    BoolMask2D xzSlice(int y) {
        auto nHist = _dims[0] * _dims[2];
        std::vector<bool> mask(nHist);
        for (auto x = 0; x < _dims[0]; ++x)
        for (auto z = 0; z < _dims[2]; ++z) {
            mask[x + _dims[0] * z] = isSelected(x, y, z);
        }
        return BoolMask2D(Extent(_dims[0], _dims[2]), mask);
    }
    BoolMask2D yzSlice(int x) {
        auto nHist = _dims[1] * _dims[2];
        std::vector<bool> mask(nHist);
        for (auto y = 0; y < _dims[1]; ++y)
        for (auto z = 0; z < _dims[2]; ++z) {
            mask[y + _dims[1] * z] = isSelected(x, y, z);
        }
        return BoolMask2D(Extent(_dims[1], _dims[2]), mask);
    }

private:
    Extent _dims;
    std::vector<bool> _mask;
};

#endif // MASK_H
