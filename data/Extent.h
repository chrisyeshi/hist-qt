#ifndef _EXTENT_H_
#define _EXTENT_H_

#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

class Extent
{
public:
    Extent(const std::vector<int>& dim) : dimension(dim) {}
    Extent(int nDim) : dimension(nDim) {}
    template<typename... Targs>
    Extent(Targs... dims) : dimension{dims...} {}

    operator const std::vector<int>&() { return dimension; }
    std::vector<int>::iterator       begin()       { return dimension.begin(); }
    std::vector<int>::const_iterator begin() const { return dimension.begin(); }
    std::vector<int>::iterator       end()       { return dimension.end(); }
    std::vector<int>::const_iterator end() const { return dimension.end(); }
    int& operator[](int index)       { return dimension[index]; }
    int  operator[](int index) const { return dimension[index]; }
    int nElement() const {
        assert(!dimension.empty());
        int prod = 1;
        for (auto d : dimension) {
            prod *= d;
        }
        return prod;
    }
    int nDim() const { return dimension.size(); }

    int idstoflat(const std::vector<int>& ids) const {
        int sum = 0;
        for (unsigned int iSum = 0; iSum < dimension.size(); ++iSum) {
            int prod = ids[iSum];
            for (unsigned int iProd = 0; iProd < iSum; ++iProd)
                prod *= dimension[iProd];
            sum += prod;
        }
        return sum;
    }

    template<typename... Targs>
    int idstoflat(int currId, Targs... ids) const {
        const int nDim = sizeof...(ids) + 1;
        return idstoflatinternal(nDim, currId, ids...);
    }

    std::vector<int> flattoids(int flatId) const {
        std::vector<int> ids(dimension.size());
        int flat = flatId;
        for (unsigned int iId = 0; iId < dimension.size(); ++iId) {
            ids[iId] = flat % dimension[iId];
            flat = flat / dimension[iId];
        }
        return ids;
    }

    template<typename... Targs>
    void flattoids(int flatId, Targs... ids) const {
        const int nDim = sizeof...(ids);
        flattoidsinternal(nDim, flatId, ids...);
    }

private:
    int idstoflatinternal(const int) const { return 0; }
    template<typename... Targs>
    int idstoflatinternal(const int nDim, int currId, Targs... ids) const {
        const int iSum = nDim - sizeof...(ids) - 1;
        assert(currId < dimension[iSum]);
        int prod = currId;
        for (int iProd = 0; iProd < iSum; ++iProd)
            prod *= dimension[iProd];
        return prod + idstoflatinternal(nDim, ids...);
    }

    void flattoidsinternal(const int, int) const { return; }
    template<typename... Targs>
    void flattoidsinternal(
            const int nDim, int flatId, int* currId, Targs... ids) const {
        const int iId = nDim - sizeof...(ids) - 1;
        *currId = flatId % dimension[iId];
        flattoidsinternal(nDim, flatId / dimension[iId], ids...);
    }

private:
    std::vector<int> dimension;
};

#endif // _EXTENT_H_
