#ifndef HISTGRID_H
#define HISTGRID_H

#include <iostream>
#include <vector>
#include "Extent.h"
#include "Histogram.h"

/// TODO: move per histogram variables to the histogram class.
struct HistHelper
{
    HistHelper();
    static HistHelper merge(
            const int direction, const HistHelper& a, const HistHelper& b);
    Extent dimHists() const { return Extent(nh_x, nh_y, nh_z); }
    int N_HIST, n_nonempty_bins, n_vx, n_vy, n_vz, nh_x, nh_y, nh_z;
};
std::istream& operator>>(std::istream& in, HistHelper& helper);
std::ostream& operator<<(std::ostream& out, const HistHelper& helper);

/**
 *
 */
template <typename T>
class IConstHGrid {
public:
    virtual ~IConstHGrid() {}
    virtual Extent dimHists() const = 0;
    virtual std::shared_ptr<const T> hist(int flatId) const = 0;
    virtual std::shared_ptr<const T> hist(
            const std::vector<int>& ids) const {
        return this->hist(dimHists().idstoflat(ids));
    }
    template<typename... Targs> std::shared_ptr<const T> hist(
            Targs... ids) const {
        return this->hist(dimHists().idstoflat(ids...));
    }
};

/**
 *
 */
template <typename T>
class IHGrid : public IConstHGrid<T> {
public:
    virtual ~IHGrid() {}
    virtual HistHelper helper() const = 0;
    int nHist() const { return helper().N_HIST; }
    virtual Extent dimHists() const override { return helper().dimHists(); }
    virtual std::shared_ptr<T> hist(int flatId) = 0;
    virtual std::shared_ptr<T> hist(const std::vector<int>& ids) {
        return this->hist(dimHists().idstoflat(ids));
    }
    template<typename... Targs> std::shared_ptr<T> hist(Targs... ids) {
        return this->hist(dimHists().idstoflat(ids...));
    }
    using IConstHGrid<T>::hist;
};

/**
 * @brief IConstHistGrid
 */
typedef IConstHGrid<Hist> IConstHistGrid;
typedef IHGrid<Hist> IHistGrid;

/**
 * @brief The HistGrid class
 */
class HistGrid : public IHistGrid
{
public:
    HistGrid() = default;
    HistGrid(HistHelper helper, std::vector<std::shared_ptr<Hist>> hists)
      : m_helper(helper), m_hists(hists) {}
    virtual ~HistGrid() {}

public:
    virtual HistHelper helper() const { return m_helper; }
    virtual std::shared_ptr<Hist> hist(int flatId) { return m_hists[flatId]; }
    virtual std::shared_ptr<const Hist> hist(int flatId) const {
        return m_hists[flatId];
    }
    using IHistGrid::hist;

protected:
    HistHelper m_helper;
    std::vector<std::shared_ptr<Hist> > m_hists;
};

/**
 * @brief The HistRect class
 */
class HistRect : public IConstHistGrid {
public:
    HistRect() : _nHistX(0), _nHistY(0) {}
    HistRect(int nHistX, int nHistY,
            std::vector<std::shared_ptr<const Hist>> hists);

public:
    virtual Extent dimHists() const override {
        return Extent(_nHistX, _nHistY);
    }
    virtual std::shared_ptr<const Hist> hist(int flatId) const override {
        return _hists[flatId];
    }
    using IConstHistGrid::hist;

public:
    int nHistX() const { return _nHistX; }
    int nHistY() const { return _nHistY; }
    int nHist() const { return nHistX() * nHistY(); }

private:
    int _nHistX, _nHistY;
    std::vector<std::shared_ptr<const Hist>> _hists;
};

/**
 * @brief The HistDomain class
 */
class HistDomain : public HistGrid
{
public:
    HistDomain() = default;
    HistDomain(HistHelper helper, std::vector<std::shared_ptr<Hist>> hists);
    HistDomain(const std::string& dir, const std::string& name, int iProc,
            const std::vector<std::string>& vars);
    ~HistDomain() {}

public:
    using HistGrid::hist;
};

/**
 * @brief The HistVolume class
 */
class HistVolume : public IHistGrid
{
public:
    HistVolume(const std::string& dir, const std::string& name,
            const std::vector<int>& dim, const std::vector<std::string>& vars);
    ~HistVolume() {}

public:
    virtual HistHelper helper() const;
    virtual std::shared_ptr<Hist> hist(int flatId);
    virtual std::shared_ptr<const Hist> hist(int flatId) const;
    using IHistGrid::hist;

public:
    std::shared_ptr<HistDomain> domain(int flatId) { return m_domains[flatId]; }
    std::shared_ptr<const HistDomain> domain(int flatId) const { return m_domains[flatId]; }
    virtual std::shared_ptr<HistDomain> domain(const std::vector<int>& ids) { return this->domain(Extent(m_dimDomains).idstoflat(ids)); }
    virtual std::shared_ptr<const HistDomain> domain(const std::vector<int>& ids) const { return this->domain(Extent(m_dimDomains).idstoflat(ids)); }
    template<typename... Targs> std::shared_ptr<HistDomain> domain(Targs... ids) { return this->domain(Extent(m_dimDomains).idstoflat(ids...)); }
    template<typename... Targs> std::shared_ptr<const HistDomain> domain(Targs... ids) const { return this->domain(Extent(m_dimDomains).idstoflat(ids...)); }
    const std::vector<int> & dimDomains() const { return m_dimDomains; }
    int nDomains() const;

public:
    std::shared_ptr<HistRect> xySlice(int z) const;
    std::shared_ptr<HistRect> xzSlice(int y) const;
    std::shared_ptr<HistRect> yzSlice(int x) const;

public:
    const std::string& dir() const { return m_dir; }
    std::vector<int> dhtoids(const std::vector<int>& dIds, const std::vector<int>& hIds) const;
    std::vector<int> dhtoids(int dId, int hId) const;
    void dhtoids(int dId, int hId, int* x, int* y, int* z) const;
    int dhtoflat(const std::vector<int>& dIds, const std::vector<int>& hIds) const;
    int dhtoflat(int dId, int hId) const;

private:
    std::vector<std::shared_ptr<HistDomain> > m_domains;
    std::vector<int> m_dimDomains;
    std::string m_dir, m_name;
    std::vector<std::string> m_vars;

private:
    mutable bool m_helperCached;
    mutable HistHelper m_helper;
};

#endif // HISTGRID_H
