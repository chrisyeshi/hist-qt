#ifndef HISTFACADEGRID_H
#define HISTFACADEGRID_H

#include <data/histgrid.h>
#include <data/dataconfigreader.h>
#include <histfacade.h>

typedef IConstHGrid<HistFacade> IConstHistFacadeGrid;
typedef IHGrid<HistFacade> IHistFacadeGrid;

/**
 * @brief The HistFacadeGrid class
 */
class HistFacadeGrid : public IHistFacadeGrid {
public:
    HistFacadeGrid() = default;
    HistFacadeGrid(
            HistHelper helper, std::vector<std::shared_ptr<HistFacade>> hists)
      : _helper(helper), _hists(hists) {}

public:
    virtual HistHelper helper() const { return _helper; }
    virtual std::shared_ptr<HistFacade> hist(int flatId) {
        return _hists[flatId];
    }
    virtual std::shared_ptr<const HistFacade> hist(int flatId) const {
        return _hists[flatId];
    }
    using IHistFacadeGrid::hist;

protected:
    HistHelper _helper;
    std::vector<std::shared_ptr<HistFacade>> _hists;
};

/**
 * @brief The HistFacadeRect class
 */
class HistFacadeRect : public IConstHistFacadeGrid {
public:
    HistFacadeRect() : _nHistX(0), _nHistY(0) {}
    HistFacadeRect(int nHistX, int nHistY,
            std::vector<std::shared_ptr<const HistFacade>> hists)
      : _nHistX(nHistX), _nHistY(nHistY), _hists(hists) {}

public:
    virtual Extent dimHists() const override {
        return Extent(_nHistX, _nHistY);
    }
    virtual std::shared_ptr<const HistFacade> hist(int flatId) const override {
        return _hists[flatId];
    }
    using IConstHistFacadeGrid::hist;

public:
    int nHistX() const { return _nHistX; }
    int nHistY() const { return _nHistY; }
    int nHist() const { return nHistX() * nHistY(); }

private:
    int _nHistX, _nHistY;
    std::vector<std::shared_ptr<const HistFacade>> _hists;
};

/**
 * @brief The HistFacadeDomain class
 */
class HistFacadeDomain : public HistFacadeGrid {
public:
    HistFacadeDomain() = default;
    HistFacadeDomain(
            HistHelper helper, std::vector<std::shared_ptr<HistFacade>> hists)
      : HistFacadeGrid(helper, hists) {}
    HistFacadeDomain(const std::string& dir, const std::string& name,
            int iDomain, const std::vector<std::string>& vars);

public:
    using HistFacadeGrid::hist;
};

/**
 * @brief The HistFacadeVolume class
 */
class HistFacadeVolume : public IHistFacadeGrid {
public:
    HistFacadeVolume(const std::string& dir, const std::string& name,
            std::vector<int> dims, const std::vector<std::string>& vars);
    HistFacadeVolume(std::string dir, std::string name,
            const MultiBlockTopology &topo, std::vector<std::string> vars);

public:
    virtual HistHelper helper() const override;
    virtual std::shared_ptr<HistFacade> hist(int flatId) override;
    virtual std::shared_ptr<const HistFacade> hist(int flatId) const override;
    using IHistFacadeGrid::hist;

public:
    std::shared_ptr<HistFacadeDomain> domain(int flatId);
    std::shared_ptr<const HistFacadeDomain> domain(int flatId) const;
    std::shared_ptr<HistFacadeDomain> domain(const std::vector<int>& ids);
    std::shared_ptr<const HistFacadeDomain> domain(
            const std::vector<int>& ids) const;
    template <typename... Args>
    std::shared_ptr<HistFacadeDomain> domain(Args... ids) {
        return domain(_dimDomains.idstoflat(ids...));
    }
    template <typename... Args>
    std::shared_ptr<const HistFacadeDomain> domain(Args... ids) const {
        return domain(_dimDomains.idstoflat(ids...));
    }
    const Extent& dimDomains() const { return _dimDomains; }
    int nDomains() const;

public:
    std::shared_ptr<HistFacadeRect> xySlice(int z) const;
    std::shared_ptr<HistFacadeRect> xzSlice(int y) const;
    std::shared_ptr<HistFacadeRect> yzSlice(int x) const;

public:
    const std::string& dir() const { return _dir; }
    const std::vector<std::string>& vars() const { return _vars; }
    std::vector<int> dhtoids(
            const std::vector<int>& dIds, const std::vector<int>& hIds) const;
    std::vector<int> dhtoids(int dId, int hId) const;
    void dhtoids(int dId, int hId, int* x, int* y, int* z) const;
    int dhtoflat(
            const std::vector<int>& dIds, const std::vector<int>& hIds) const;
    int dhtoflat(int dId, int hId) const;

private:
    std::vector<std::shared_ptr<HistFacadeDomain>> _domains;
    Extent _dimDomains;
    std::string _dir, _name;
    std::vector<std::string> _vars;

private:
    mutable bool _helperCached;
    mutable HistHelper _helper;
};

#endif // HISTFACADEGRID_H
