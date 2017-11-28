#include "Histogram.h"

#include <fstream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <algorithm>
#include "histreader.h"

bool operator==(const Interval<float> &a, const Interval<float> &b)
{
    if (fabs(a.lower - b.lower) > 0.0001)
        return false;
    if (fabs(a.upper - b.upper) > 0.0001)
        return false;
    return true;
}

std::ostream& operator<<(std::ostream& out, const Hist& hist)
{
    assert(hist.nDim() > 0);
    int ix = 0;
    std::cout << "nBins = " << hist.nBins() << std::endl;
    for (int flatId = 0; flatId < hist.nBins(); ++flatId) {
        out << std::setw(4) << hist.binFreq(flatId) << " ";
        if (++ix >= hist.dim()[0]) {
            ix = 0;
            out << std::endl;
        }
    }
    return out;
}

Hist* Hist::fromDenseValues(int ndim, const std::vector<int>& nbins,
        const std::vector<double>& mins, const std::vector<double>& maxs,
        const std::vector<double>& logBases,
        const std::vector<std::string>& vars,
        const std::vector<float> &values) {
    if (3 == ndim) {
        return new Hist3DFull(nbins[0], nbins[1], nbins[2], mins, maxs,
                logBases, vars, values);
    }
    if (2 == ndim) {
        return new Hist2D(
                nbins[0], nbins[1], mins, maxs, logBases, vars, values);
    }
    if (1 == ndim) {
        return new Hist1D(
                nbins[0], mins[0], maxs[0], logBases[0], vars[0], values);
    }
    assert(false);
    return nullptr;
}

Hist *Hist::fromBuffer(bool isSparse, int ndim, const std::vector<int> &nbins,
        const std::vector<double> &mins, const std::vector<double> &maxs,
        const std::vector<double> &logBases,
        const std::vector<std::string> &vars, const std::vector<int> &buffer) {
    if (isSparse && ndim == 3) {
        std::vector<int> binIds;
        std::vector<float> values;
        for (unsigned int i = 0; i < buffer.size(); i += 2) {
            binIds.push_back(buffer[i]);
            values.push_back(float(buffer[i + 1]));
        }
        return new Hist3DSparse(nbins[0], nbins[1], nbins[2], mins, maxs,
                logBases, vars, binIds, values);
    }
    if (isSparse && ndim == 2) {
        std::vector<int> binIds;
        std::vector<float> values;
        for (unsigned int i = 0; i < buffer.size(); i += 2) {
            binIds.push_back(buffer[i]);
            values.push_back(float(buffer[i + 1]));
        }
        return new Hist2D(nbins[0], nbins[1], mins, maxs, logBases, vars,
                binIds, values);
    }
    if (isSparse && ndim == 1) {
        std::vector<int> binIds;
        std::vector<float> values;
        for (unsigned int i = 0; i < buffer.size(); i += 2) {
            binIds.push_back(buffer[i]);
            values.push_back(float(buffer[i + 1]));
        }
        return new Hist1D(nbins[0], mins[0], maxs[0], logBases[0], vars[0],
                binIds, values);
    }
    // dense representation
    assert(!isSparse);
    std::vector<float> values(buffer.size());
    for (unsigned int i = 0; i < buffer.size(); ++i) {
        values[i] = float(buffer[i]);
    }
    return fromDenseValues(ndim, nbins, mins, maxs, logBases, vars, values);
}

HistBin Hist::binSum() const
{
    float value = 0.0;
    float percent = 0.f;
    for (int iBin = 0; iBin < nBins(); ++iBin) {
        value += binFreq(iBin);
        percent += binPercent(iBin);
    }
    return HistBin(value, percent);
}

HistBin Hist::binSum(std::vector<std::pair<int, int> > binRanges) const
{
    std::vector<int> iBins(binRanges.size());
    std::vector<int> nBins(binRanges.size());

    const int BIN_RANGE_SZ = binRanges.size();

    int nBin = 1;
    for (int iDim = 0; iDim < BIN_RANGE_SZ; ++iDim)
    {
        iBins[iDim] = binRanges[iDim].first;
        nBins[iDim] = binRanges[iDim].second - binRanges[iDim].first + 1;
        nBin *= nBins[iDim];
    }

    double value = 0.0;
    float percent = 0.f;
    std::vector<int> incs(binRanges.size(), 0);
    for (int iBin = 0; iBin < nBin; ++iBin) {
        value += binFreq(iBins);
        percent += binPercent(iBins);
        // increment (so complicating... Orz)
        std::fill(incs.begin(), incs.end(), 0);
        incs[0] = 1;

        for (int iDim = 0; iDim < BIN_RANGE_SZ - 1; ++iDim) {
            incs[iDim + 1] =
                    (iBins[iDim] + incs[iDim] - binRanges[iDim].first) /
                    (nBins[iDim]);
            iBins[iDim] =
                    binRanges[iDim].first +
                    (iBins[iDim] + incs[iDim] - binRanges[iDim].first) %
                    (nBins[iDim]);
        }
        iBins[BIN_RANGE_SZ - 1] += incs[BIN_RANGE_SZ - 1];
    }
    return HistBin(value, percent);
}

bool Hist::checkRange(
        std::vector<std::pair<int, int>> binRanges, float threshold) const
{
    return binSum(binRanges).percent() > threshold / 100.f;
}

bool Hist::checkRange(
        const std::vector<Interval<float>> &intervals, float threshold) const
{
    // get the bin ranges from the normalized intervals
    std::vector<std::pair<int,int>> binRanges(nDim());
    for (int iDim = 0; iDim < nDim(); ++iDim) {
        const Interval<float>& interval = intervals[iDim];
        /// TODO: verify this
        binRanges[iDim].first = std::ceil(interval.lower * float(m_dim[iDim]));
        binRanges[iDim].second =
                std::floor(interval.upper * float(m_dim[iDim]));
    }
    // use the other version of checkRange to do the real work
    return checkRange(binRanges, threshold * 100.f);
}


//////////////////////////////////////////////////////////////////////////////
// Hist1D

Hist1D::Hist1D(int dim, double min, double max, double logBase,
        const std::string &var, const std::vector<float> &values)
  : Hist(1, {min}, {max}, {logBase}, {var})
  , m_values(values)
  , m_sum(0.0) {
    assert(dim == int(values.size()));
    m_dim[0] = dim;
    for (float value : values)
        m_sum += value;
}

Hist1D::Hist1D(int dim, double min, double max, double logBase,
        const std::string& var, const std::vector<int> &binIds,
        const std::vector<float> &values)
  : Hist(1, {min}, {max}, {logBase}, {var})
  , m_values(dim, 0.0)
  , m_sum(0.0)
{
    m_dim[0] = dim;
    for (unsigned int iBin = 0; iBin < binIds.size(); ++iBin) {
        m_values[binIds[iBin]] = values[iBin];
        m_sum += values[iBin];
    }
}


////////////////////////////////////////////////////////////////////////////////////////////
// Hist2D

/// TODO: consider merging dimx, dimy into an array.
/// TODO: consider merging the following two constructors with the Hist1D constructors.
Hist2D::Hist2D(int dimx, int dimy,
        const std::vector<double> &mins, const std::vector<double> &maxs,
        const std::vector<double> &logBases, const std::vector<std::string> &vars,
        const std::vector<float> &values)
  : Hist(2, mins, maxs, logBases, vars)
  , m_values(values)
  , m_sum(0.0)
{
    assert(dimx * dimy == int(values.size()));
    m_dim[0] = dimx;
    m_dim[1] = dimy;
    for (float value : values)
        m_sum += value;
}

Hist2D::Hist2D(int dimx, int dimy,
        const std::vector<double> &mins, const std::vector<double> &maxs,
        const std::vector<double> &logBases, const std::vector<std::string> &vars,
        const std::vector<int> &binIds, const std::vector<float> &values)
  : Hist(2, mins, maxs, logBases, vars)
  , m_values(dimx * dimy, 0.0)
  , m_sum(0.0)
{
    m_dim[0] = dimx;
    m_dim[1] = dimy;
    for (unsigned int iBin = 0; iBin < binIds.size(); ++iBin) {
        m_values[binIds[iBin]] = values[iBin];
        m_sum += values[iBin];
    }
}

Hist2D::Hist2D(Hist2D&& hist)
  : Hist(2, hist.m_mins, hist.m_maxs, hist.m_logBases, hist.m_vars)
{
    m_dim[0] = hist.m_dim[0];
    m_dim[1] = hist.m_dim[1];
    m_values = std::move(hist.m_values);
    m_sum = hist.m_sum;
}

Hist1D Hist2D::to1D(int dimidx) const
{
    int dimidy = 1 - dimidx;
    int dimx = m_dim[dimidx];
    int dimy = m_dim[dimidy];
    double min = m_mins[dimidx], max = m_maxs[dimidx];
    double logBase = m_logBases[dimidx];
    std::string var = m_vars[dimidx];
    std::vector<float> values(dimx);
    std::vector<int> binId2(2);
    for (int x = 0; x < dimx; ++x) {
        binId2[dimidx] = x;
        for (int y = 0; y < dimy; ++y) {
            binId2[dimidy] = y;
            values[x] += binFreq(binId2);
        }
    }
    return Hist1D(dimx, min, max, logBase, var, values);
}

std::shared_ptr<Hist1D> Hist2D::to1DPtr(int dimidx) const
{
    return std::make_shared<Hist1D>(to1D(dimidx));
}

bool Hist2D::checkRange(std::vector<std::pair<int32_t, int32_t>> binRanges,
        float threshold) const {
    float sum = 0;
    for(int xI = binRanges[0].first; xI <= binRanges[0].second; ++xI)
    for(int yI = binRanges[1].first; yI <= binRanges[1].second; ++yI ) {
        sum += binPercent(xI, yI);
    }
    return sum*100 >= threshold;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Hist3D


bool Hist3D::checkRange( std::vector< std::pair< int32_t, int32_t > > binRanges, float threshold ) const
{
    float sum = 0;
    for(int xI = binRanges[0].first; xI <= binRanges[0].second; ++xI)
    for(int yI = binRanges[1].first; yI <= binRanges[1].second; ++yI)
    for(int zI = binRanges[2].first; zI <= binRanges[2].second; ++zI) {
        sum += binPercent(xI, yI, zI);
    }
    return sum*100 >= threshold;
}

std::shared_ptr<Hist3D> Hist3D::create(int dimx, int dimy, int dimz,
        const std::vector<double> &mins, const std::vector<double> &maxs,
        const std::vector<double> &logBases, const std::vector<int> &binIds,
        const std::vector<float> &values, const std::vector<std::string> &vars)
{
    return std::make_shared<Hist3DSparse>(
            dimx, dimy, dimz, mins, maxs, logBases, vars, binIds, values);
}

Hist2D Hist3D::to2D(int dimidx, int dimidy) const
{
    int dimidz = 3 - dimidx - dimidy;
    int dimx = m_dim[dimidx];
    int dimy = m_dim[dimidy];
    int dimz = m_dim[dimidz];
    std::vector<double> mins = { m_mins[dimidx], m_mins[dimidy] };
    std::vector<double> maxs = { m_maxs[dimidx], m_maxs[dimidy] };
    std::vector<double> logBases = { m_logBases[dimidx], m_logBases[dimidy] };
    std::vector<std::string> vars = { m_vars[dimidx], m_vars[dimidy] };

    std::vector<float> values(dimx * dimy, 0.0);
    std::vector<int> binId3(3);
    for (int y = 0; y < dimy; ++y)
    for (int x = 0; x < dimx; ++x)
    {
        binId3[dimidx] = x;
        binId3[dimidy] = y;
        int flatId = x + dimx * y;
        for (int z = 0; z < dimz; ++z)
        {
            binId3[dimidz] = z;
            values[flatId] += binFreq(binId3);
        }
    }

    return Hist2D(dimx, dimy, mins, maxs, logBases, vars, values);
}

std::shared_ptr<Hist2D> Hist3D::to2DPtr(int dimidx, int dimidy) const
{
    return std::make_shared<Hist2D>(to2D(dimidx, dimidy));
}


Hist3DFull::Hist3DFull(int dimx, int dimy, int dimz,
        const std::vector<double> &mins, const std::vector<double> &maxs,
        const std::vector<double> &logBases, const std::vector<std::string>& vars,
        const std::vector<float> &values)
  : Hist3D(dimx, dimy, dimz, mins, maxs, logBases, vars)
  , m_values(values)
  , m_sum(0.0)
{
    for (float value : values)
        m_sum += value;
}

std::shared_ptr<Hist> Hist3DFull::toSparse()
{
    std::vector<int> binIds;
    std::vector<float> values;
    for (unsigned int binId = 0; binId < m_values.size(); ++binId)
    {
        if (m_values[binId] > 0.0001)
        {
            binIds.push_back(binId);
            values.push_back(m_values[binId]);
        }
    }
    return std::make_shared<Hist3DSparse>(
            m_dim[0], m_dim[1], m_dim[2], m_mins, m_maxs, m_logBases, m_vars,
            binIds, values);
}

//HistBin Hist3DFull::bin(const int flatId) const
//{
//    assert(flatId < int(m_values.size()));
//    return HistBin(m_values[flatId], float(m_values[flatId]) / float(m_sum));
//}





Hist3DSparse::Hist3DSparse(int dimx, int dimy, int dimz,
        const std::vector<double> &mins, const std::vector<double> &maxs,
        const std::vector<double> &logBases, const std::vector<std::string> &vars,
        const std::vector<int>& binIds, const std::vector<float> &values)
  : Hist3D(dimx, dimy, dimz, mins, maxs, logBases, vars)
  , m_binIds(binIds)
  , m_values(values)
  , m_sum(0.0)
{
    for (float value : m_values)
        m_sum += value;
}

bool Hist3DSparse::checkRange( std::vector< std::pair< int32_t, int32_t > > binRanges, float threshold ) const
{
    int x, y, z;
    float sum = 0;
    const int SZ = m_values.size();
    for( int i = 0; i < SZ; ++i )
    {
        m_dim.flattoids( m_binIds[ i ], &x, &y, &z );
        if( x >= binRanges[ 0 ].first && x <= binRanges[ 0 ].second
         && y >= binRanges[ 1 ].first && y <= binRanges[ 1 ].second
         && z >= binRanges[ 2 ].first && x <= binRanges[ 2 ].second )
        {
            sum += m_values[ i ];
        }
    }
    return sum / m_sum  >= threshold / 100.f;
}

std::shared_ptr<Hist> Hist3DSparse::toFull()
{
    std::vector<float> values(m_dim[0] * m_dim[1] * m_dim[2]);

    /// TODO: obviously there is a faster way to do this
    for (unsigned int binId = 0; binId < values.size(); ++binId)
        values[binId] = binFreq(binId);

    return std::make_shared<Hist3DFull>(
            m_dim[0], m_dim[1], m_dim[2], m_mins, m_maxs, m_logBases, m_vars, values);
}

//HistBin Hist3DSparse::bin(const int flatId) const
//{
//    // find if the bin has anything
//    int arrayIndex = -1;
//    for (unsigned int i = 0; i < m_binIds.size(); ++i)
//    {
//        if (flatId == m_binIds[i])
//        {
//            arrayIndex = i;
//            break;
//        }
//    }
//    // id not found
//    if (-1 == arrayIndex)
//        return HistBin(0.0, 0.f);
//    // id found
//    return HistBin(m_values[arrayIndex], m_values[arrayIndex] / m_sum);
//}

float Hist3DSparse::binFreq(const int flatId) const {
    auto itr = std::find(m_binIds.begin(), m_binIds.end(), flatId);
    if (m_binIds.end() == itr) {
        return 0.0;
    }
    return *itr;
}

std::shared_ptr<const Hist> HistCollapser::collapseTo(
        const std::vector<int>& dims) {
    assert(int(dims.size()) <= m_hist->nDim() && !dims.empty());
    if (int(dims.size()) == m_hist->nDim()) return m_hist;
    if (int(dims.size()) == 2 && m_hist->nDim() == 3) {
        auto hist3D = std::dynamic_pointer_cast<const Hist3D>(m_hist);
        return hist3D->to2DPtr(dims[0], dims[1]);
    }
    if (dims.size() == 1 && m_hist->nDim() == 3) {
        auto hist3D = std::dynamic_pointer_cast<const Hist3D>(m_hist);
        return hist3D->to2DPtr(dims[0], (dims[0] + 1) % 3)->to1DPtr(0);
    }
    if (dims.size() == 1 && m_hist->nDim() == 2) {
        auto hist2D = std::dynamic_pointer_cast<const Hist2D>(m_hist);
        return hist2D->to1DPtr(dims[0]);
    }
    return nullptr;
}
