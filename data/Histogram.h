#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#include <memory>
#include <iostream>
#include <vector>
#include <limits>
#include "Extent.h"




/// TODO: move it somewhere meaningful
template <typename T>
class Interval {
public:
    T lower, upper;
};
bool operator==(const Interval<float>& a, const Interval<float>& b);





class HistBin
{
public:
    HistBin(float value, float percent) : m_value(value), m_percent(percent) {}
    ~HistBin() {}

public:
    float value() const { return m_value; }
    float percent() const { return m_percent; }

private:
    float m_value;
    float m_percent;
};





class Hist : public std::enable_shared_from_this<Hist>
{
public:
    static Hist* fromDenseValues(int ndim, const std::vector<int>& nbins,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars,
            const std::vector<float>& values);
    static Hist* fromBuffer(bool isSparse, int ndim,
            const std::vector<int>& nbins,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars,
            const std::vector<int>& buffer);

public:
    Hist(int nDim, const std::vector<double>& mins,
            const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars)
      : m_dim(nDim), m_mins(mins), m_maxs(maxs), m_logBases(logBases)
      , m_vars(vars) {}
    virtual ~Hist() {}

public:
    virtual std::shared_ptr<Hist> toSparse() = 0;
    /// TODO: change toFull to toDense as it goes better with the terminology
    virtual std::shared_ptr<Hist> toFull() = 0;
    virtual float binFreq(const int flatId) const = 0;
    virtual float binFreq(const std::vector<int>& ids) const {
        return binFreq(m_dim.idstoflat(ids));
    }
    template <typename... Targs>
    float binFreq(int currId, Targs... ids) const {
        return binFreq(m_dim.idstoflat(currId, ids...));
    }
    virtual float binPercent(const int flatId) const = 0;
    virtual float binPercent(const std::vector<int>& ids) const {
        return binPercent(m_dim.idstoflat(ids));
    }
    template <typename... Targs>
    float binPercent(int currId, Targs... ids) const {
        return binPercent(m_dim.idstoflat(currId, ids...));
    }
    virtual std::vector<std::array<double, 2>> binRanges(
            const int flatId) const {
        auto ids = m_dim.flattoids(flatId);
        std::vector<std::array<double, 2>> ranges(ids.size());
        for (auto iDim = 0; iDim < ids.size(); ++iDim) {
            auto min = m_mins[iDim];
            auto max = m_maxs[iDim];
            auto range = max - min;
            auto nBin = m_dim[iDim];
            auto binRange = range / nBin;
            ranges[iDim][0] = min + (ids[iDim] + 0) * binRange;
            ranges[iDim][1] = min + (ids[iDim] + 1) * binRange;
        }
        return ranges;
    }
    virtual std::vector<std::array<double, 2>> binRanges(
            const std::vector<int>& ids) const {
        return binRanges(m_dim.idstoflat(ids));
    }
    template <typename... Targs>
    std::vector<std::array<double, 2>> binRanges(
            int currId, Targs... ids) const {
        return binRanges(m_dim.idstoflat(currId, ids...));
    }
    virtual HistBin varRangesValue(
            const std::vector<std::array<double, 2>>& varRanges) const {}
    std::vector<float> idsOfValuesF(std::vector<double> valueTuple) const;

    virtual const std::vector<float>& values() const {
        std::cout << "Are you sure the histogram is in dense representation?"
                  << std::endl;
        static std::vector<float> hehe;
        return hehe;
    }
    virtual HistBin binSum() const;
    virtual HistBin binSum(std::vector<std::pair<int, int> > binRanges) const;
    virtual bool checkRange(std::vector< std::pair<int, int> > binRanges,
            float threshold) const;
    virtual bool checkRange(const std::vector<Interval<float>>& intervals,
            float threshold) const;
    virtual std::vector<float> means() const;

public:
    const Extent& dim() const { return m_dim; }
    int nDim() const { return dim().nDim(); }
    int nBins() const {
        return [this](){
            int prod = 1;
            for (auto dim : m_dim) prod *= dim;
            return prod;
        }();
    }
    virtual double dimMin(int iDim) const {
        assert(iDim < m_dim.nDim()); return m_mins[iDim];
    }
    virtual double dimMax(int iDim) const {
        assert(iDim < m_dim.nDim()); return m_maxs[iDim];
    }
    double logBase(int iDim) const {
        assert(iDim < m_dim.nDim()); return m_logBases[iDim];
    }
    const std::string& var(int iDim) const {
        assert(iDim < m_dim.nDim()); return m_vars[iDim];
    }
    const std::vector<std::string>& vars() const { return m_vars; }

protected:
    Extent m_dim;
    std::vector<double> m_mins, m_maxs, m_logBases;
    std::vector<std::string> m_vars;
};
std::ostream& operator<<(std::ostream& out, const Hist& hist);



class HistNull : public Hist {
public:
    HistNull() : Hist(0, {}, {}, {}, {}) {}
    virtual std::shared_ptr<Hist> toSparse() override {
        return shared_from_this();
    }
    virtual std::shared_ptr<Hist> toFull() override {
        return shared_from_this();
    }

//    virtual HistBin bin(const int flatId) const override {
//        return HistBin(-1.0, -1.f);
//    }
//    using Hist::bin;

    virtual float binFreq(const int flatId) const override { return -1.0; }
    using Hist::binFreq;
    virtual float binPercent(const int flatId) const override { return -1.f; }
    using Hist::binPercent;

    virtual const std::vector<float>& values() const override {
        static std::vector<float> v;
        return v;
    }
    virtual double dimMin(int) const override {
        return std::numeric_limits<double>::max();
    }
    virtual double dimMax(int) const override {
        return std::numeric_limits<double>::lowest();
    }
    virtual std::vector<float> means() const override { return {}; }
};



/// TODO: only dense representation for 1D histogram for now.
class Hist1D : public Hist
{
public:
    Hist1D(int dim, double min, double max, double logBase,
            const std::string& var, const std::vector<float>& values);
    Hist1D(int dim, double min, double max, double logBase,
            const std::string& var, const std::vector<int>& binIds,
            const std::vector<float>& values);

public:
    virtual std::shared_ptr<Hist> toSparse() { return shared_from_this(); }
    virtual std::shared_ptr<Hist> toFull() { return shared_from_this(); }

    virtual float binFreq(const int flatId) const override {
        return m_values[flatId];
    }
    using Hist::binFreq;
    virtual float binPercent(const int flatId) const override {
        return m_values[flatId] / m_sum;
    }
    using Hist::binPercent;
    virtual HistBin varRangesValue(
            const std::vector<std::array<double, 2>>& varRanges) const override;

    virtual const std::vector<float>& values() const { return m_values; }
    virtual std::vector<float> means() const override;

private:
    std::vector<float> m_values;
    float m_sum;
};




/// TODO: only dense version for Hist2D for now
class Hist2D : public Hist
{
public:
    Hist2D(int dimx, int dimy,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars,
            const std::vector<float>& values);
    Hist2D(int dimx, int dimy,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars,
            const std::vector<int>& binIds, const std::vector<float>& values);
    Hist2D(Hist2D&& hist);
    virtual ~Hist2D() {}

public:
    Hist1D to1D(int dimidx) const;
    std::shared_ptr<Hist1D> to1DPtr(int dimidx) const;
    virtual std::shared_ptr<Hist> toSparse() { return shared_from_this(); }
    virtual std::shared_ptr<Hist> toFull() { return shared_from_this(); }

    virtual float binFreq(const int flatId) const override {
        return m_values[flatId];
    }
    using Hist::binFreq;
    virtual float binPercent(const int flatId) const override {
        return m_values[flatId] / m_sum;
    }
    using Hist::binPercent;

    virtual HistBin varRangesValue(
            const std::vector<std::array<double, 2>>& varRanges) const override;

    virtual const std::vector<float>& values() const { return m_values; }
    virtual bool checkRange(std::vector<std::pair<int32_t, int32_t>> binRanges,
            float threshold) const;

private:
    std::vector<float> m_values;
    float m_sum;
};






class Hist3D : public Hist
{
public:
    /// TODO: use a vector to substitute dimx, dimy, dimz.
    Hist3D(int dimx, int dimy, int dimz,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars)
      : Hist(3, mins, maxs, logBases, vars) {
        m_dim[0] = dimx; m_dim[1] = dimy; m_dim[2] = dimz;
    }
    static std::shared_ptr<Hist3D> create(int dimx, int dimy, int dimz,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases, const std::vector<int>& binIds,
            const std::vector<float>& values,
            const std::vector<std::string>& vars);
    virtual ~Hist3D() {}

public:
    Hist2D to2D(int dimidx, int dimidy) const;
    std::shared_ptr<Hist2D> to2DPtr(int dimidx, int dimidy) const;
    virtual bool checkRange(std::vector<std::pair<int32_t, int32_t>> binRanges,
            float threshold ) const;
};






class Hist3DFull : public Hist3D
{
public:
    Hist3DFull(int dimx, int dimy, int dimz,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars,
            const std::vector<float>& values);
    virtual ~Hist3DFull() {}

public:
    virtual std::shared_ptr<Hist> toSparse();
    virtual std::shared_ptr<Hist> toFull() { return shared_from_this(); }

//    virtual HistBin bin(const int flatId) const;
//    using Hist3D::bin;

    virtual float binFreq(const int flatId) const override {
        return m_values[flatId];
    }
    using Hist3D::binFreq;
    virtual float binPercent(const int flatId) const override {
        return m_values[flatId] / m_sum;
    }
    using Hist3D::binPercent;

    virtual const std::vector<float>& values() const { return m_values; }

private:
    std::vector<float> m_values;
    float m_sum;
};





class Hist3DSparse : public Hist3D
{
public:
    Hist3DSparse(int dimx, int dimy, int dimz,
            const std::vector<double>& mins, const std::vector<double>& maxs,
            const std::vector<double>& logBases,
            const std::vector<std::string>& vars,
            const std::vector<int>& binIds, const std::vector<float>& values);
    virtual ~Hist3DSparse() {}

public:
    virtual std::shared_ptr<Hist> toSparse() { return shared_from_this(); }
    virtual std::shared_ptr<Hist> toFull();
    virtual bool checkRange(std::vector<std::pair<int32_t, int32_t>> binRanges,
            float threshold ) const;

//    virtual HistBin bin(const int flatId) const;
//    using Hist3D::bin;

    virtual float binFreq(const int flatId) const override;
    using Hist3D::binFreq;
    virtual float binPercent(const int flatId) const override {
        return binFreq(flatId) / m_sum;
    }
    using Hist3D::binPercent;

private:
    std::vector<int> m_binIds;
    std::vector<float> m_values;
    float m_sum;
};





class HistCollapser
{
public:
    HistCollapser(std::shared_ptr<const Hist> hist) : m_hist(hist) {}

public:
    std::shared_ptr<const Hist> collapseTo(const std::vector<int>& dims);

private:
    std::shared_ptr<const Hist> m_hist;
};

#endif // _HISTOGRAM_H_
