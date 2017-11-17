#include "histmerger.h"
#include <cmath>
#include <limits>
#include <vector>
#include <array>
#include <map>
#include <tuple>
#include <numeric>
#include <yy/functional.h>
#include "histbinwidthcalculator.h"

namespace {

struct Range;
std::ostream& operator<<(std::ostream& os, const Range& range);

struct Range {
public:
    static Range unionRanges(const std::vector<Range>& ranges) {
        Range res(std::numeric_limits<double>::max(),
                std::numeric_limits<double>::lowest());
        for (auto range : ranges) {
            res.lower() = std::min(range.lower(), res.lower());
            res.upper() = std::max(range.upper(), res.upper());
        }
        return res;
    }

    static Range intersectRanges(const std::vector<Range>& ranges) {
        Range res(std::numeric_limits<double>::lowest(),
                std::numeric_limits<double>::max());
        for (auto range : ranges) {
            // std::cout << range << std::endl;
            res.lower() = std::max(range.lower(), res.lower());
            res.upper() = std::min(range.upper(), res.upper());
        }
        // std::cout << res << std::endl;
//        double epsilon = std::numeric_limits<double>::epsilon();
//        assert(res.lower() <= res.upper() + 10.0 * epsilon);
        return res;
    }

public:
    Range() {}
    Range(double a, double b) : data{a, b} {}

public:
    double lower() const { return data[0]; }
    double& lower() { return data[0]; }
    double upper() const { return data[1]; }
    double& upper() { return data[1]; }
    double operator[](int index) const { return data[index]; }
    double& operator[](int index) { return data[index]; }
    double range() const { return upper() - lower(); }

private:
    std::array<double,2> data;
};

std::ostream& operator<<(std::ostream& os, const Range& range) {
    os << "range: [" << range.lower() << ", " << range.upper() << "]";
    return os;
}

template <typename A, typename B>
std::vector<std::tuple<A,B>> zip(
        const std::vector<A>& a, const std::vector<B>& b) {
    assert(a.size() == b.size());
    std::vector<std::tuple<A,B>> zipped(a.size());
    for (unsigned int i = 0; i < a.size(); ++i) {
        zipped[i] = std::make_tuple(a[i], b[i]);
    }
    return zipped;
}

template <typename T, typename... Args>
std::vector<std::tuple<T, Args...>> zip(
        const std::vector<T>& array, const std::vector<Args>&... args) {
    std::vector<std::tuple<T, Args...>> zipped(array.size());
    for (unsigned int i = 0; i < array.size(); ++i) {
        zipped[i] = std::make_tuple(array[i], args[i]...);
    }
    return zipped;
}

template <typename B, typename A>
std::vector<A> map(const std::function<A(const B&)>& functor,
        const std::vector<B>& input) {
    std::vector<A> result;
    result.reserve(input.size());
    for (auto element : input) {
        result.push_back(functor(element));
    }
    return result;
}

template <typename T>
std::vector<std::vector<T>> crossProduct(
        const std::vector<std::vector<T>>& vectors) {
    std::vector<int> ids(vectors.size(), 0);
    std::vector<T> entry(vectors.size());
    std::vector<std::vector<T>> entries;
    std::vector<int> sizes(vectors.size());
    for (unsigned int i = 0; i < vectors.size(); ++i) {
        sizes[i] = vectors[i].size();
    }
    auto inc = [sizes](std::vector<int> curr) {
        ++curr[0];
        for (unsigned int i = 0; i < curr.size() - 1; ++i) {
            if (curr[i] >= sizes[i]) {
                curr[i] = 0;
                ++curr[i + 1];
            }
        }
        return curr;
    };
    while (ids[ids.size() - 1] < sizes[sizes.size() - 1]) {
        std::vector<T> entry(ids.size());
        for (unsigned int i = 0; i < vectors.size(); ++i) {
            entry[i] = vectors[i][ids[i]];
        }
        entries.push_back(entry);
        ids = inc(ids);
    }
    return entries;
}

std::vector<Range> histsToRanges(
        int iDim, const std::vector<std::shared_ptr<const Hist>>& hists) {
    return map<std::shared_ptr<const Hist>, Range>(
            [iDim](const std::shared_ptr<const Hist>& hist) {
        return Range(hist->dimMin(iDim), hist->dimMax(iDim));
    }, hists);
}

auto calcEndBinId = [](Range binRange, Range range, double singleBinRange,
        int nBins) {
//    double delta = 10.0 * std::numeric_limits<double>::epsilon();
    double delta = 0.5 * singleBinRange;
    assert(binRange.upper() <= range.upper() + delta);
    if (range.upper() - delta <= binRange.upper()
            && binRange.upper() < range.upper() + delta) {
        return nBins - 1;
    }
    return int((binRange.upper() - range.lower()) / singleBinRange);
};

template <typename T>
T multiplyEach(const std::vector<T>& numbers) {
    assert(!numbers.empty());
    if (numbers.empty()) {
        throw "The array is empty!";
    }
    T result = numbers[0];
    for (unsigned int i = 1; i < numbers.size(); ++i)
        result *= numbers[i];
    return result;
}

std::vector<std::vector<int>> makeIdsArray(
        const std::vector<std::array<int,2>>& idIntervals) {
    std::vector<std::vector<int>> idsCols(idIntervals.size());
    for (unsigned int iDim = 0; iDim < idIntervals.size(); ++iDim) {
        auto& col = idsCols[iDim];
        col.resize(idIntervals[iDim][1] - idIntervals[iDim][0] + 1);
        std::iota(col.begin(), col.end(), idIntervals[iDim][0]);
    }
    return crossProduct(idsCols);
}

} // anonymous namespace

int HistMerger::calcBinCount(
        int iDim, const std::vector<std::shared_ptr<const Hist>>& hists) const {
    if (int(_binCounts.size()) > iDim && _binCounts[iDim]._binCount > 0) {
        return _binCounts[iDim]._binCount;
    }
    /// TODO: extract these into HistBinWidthCalculator?
    if (int(_binCounts.size()) <= iDim
            || "sturges" == _binCounts[iDim]._methodName) {
        double totalValue = 0.0;
        for (auto hist : hists) {
            totalValue += hist->binSum().value();
        }
        // apply sturge's formula
        return HistBinWidthCalculatorSturges::nBins(totalValue);
    }
    if ("freedman" == _binCounts[iDim]._methodName) {
        double totalValue = 0.0;
        for (auto hist : hists) {
            totalValue += hist->binSum().value();
        }
        double firstValue = 0.25 * totalValue;
        double thirdValue = 0.75 * totalValue;
        std::map<double, double> accumulated;
        double totalMin = std::numeric_limits<double>::max();
        double totalMax = std::numeric_limits<double>::lowest();
        for (auto hist : hists) {
            std::shared_ptr<const Hist> hist1d =
                    HistCollapser(hist).collapseTo({iDim});
            double histRange = hist1d->dimMax(0) - hist1d->dimMin(0);
            double binRange = histRange / hist1d->dim()[0];
            for (int iBin = 0; iBin < hist1d->dim()[0]; ++iBin) {
                double key = iBin * binRange + 0.5 * binRange;
                double value = hist1d->bin(iBin).value();
                if (0 == accumulated.count(key))
                    accumulated[key] = 0.0;
                accumulated[key] += value;
            }
            totalMin = std::min(hist1d->dimMin(0), totalMin);
            totalMax = std::max(hist1d->dimMax(0), totalMax);
        }
        double value = 0.0;
        double firstKey, thirdKey;
        auto itr = accumulated.begin();
        while (itr != accumulated.end()) {
            if (value <= firstValue && firstValue < value + itr->second) {
                firstKey = itr->first;
            }
            if (value <= thirdValue && thirdValue < value + itr->second) {
                thirdKey = itr->first;
            }
            value += itr->second;
            ++itr;
        }
        double iqr = thirdKey - firstKey;
        if (iqr <= 10 * std::numeric_limits<double>::epsilon()) {
            return 10;
        }
        double binWidth = 2 * iqr / std::pow(totalValue, 1.0 / 3.0);
        return std::ceil((totalMax - totalMin) /  binWidth);
    }
    assert(false);
    throw "Incorrect bin count and/or method names.";
    return -1;
}

std::vector<int> HistMerger::calcBinCounts(
        const std::vector<std::shared_ptr<const Hist>>& hists) const {
    int nDim = hists[0]->nDim();
    std::vector<int> nBins(nDim);
    for (int iDim = 0; iDim < nDim; ++iDim) {
        nBins[iDim] = calcBinCount(iDim, hists);
    }
    return nBins;
}

std::shared_ptr<Hist> HistMerger::merge(
        const std::vector<std::shared_ptr<const Hist>> &hists) const {
    // error checking
    assert(!hists.empty());
    int nDim = hists[0]->nDim();
    std::vector<std::string> vars = hists[0]->vars();
    for (auto hist : hists) {
        assert(nDim == hist->nDim());
        assert(vars == hist->vars());
    }
    // if same range and same bin widths, then merge the numbers only.

    // if different range or different bin widths, first calculate the new range
    // as the min and max, then calculates the new optimal bin widths according
    // to one of the methods, and finally put the old bins into new bins by
    // overlapping areas.
    std::vector<int> dims(nDim);
    std::iota(dims.begin(), dims.end(), 0);
    std::vector<int> nBins = calcBinCounts(hists);
    int nBin = multiplyEach(nBins);

    // std::cout << "nBin = " << nBin << std::endl;

    // get new ranges
    std::vector<Range> ranges = map<int, Range>([hists](int iDim) {
        return Range::unionRanges(histsToRanges(iDim, hists));
    }, dims);

    // std::cout << "x" << ranges[0] << std::endl;
    // std::cout << "y" << ranges[1] << std::endl;

    std::vector<double> singleBinRanges = map<std::tuple<Range, int>, double>(
            [](const std::tuple<Range, int>& rangeNBin) {
        return std::get<0>(rangeNBin).range() / double(std::get<1>(rangeNBin));
    }, zip(ranges, nBins));
    // put old values into new bins
    std::vector<double> values(nBin, 0.0);
    for (auto hist : hists) {
        std::vector<Range> histRanges = map<int, Range>([hist](int iDim) {
            return Range(hist->dimMin(iDim), hist->dimMax(iDim));
        }, dims);

        // std::cout << "xHist" << histRanges[0] << std::endl;
        // std::cout << "yHist" << histRanges[1] << std::endl;

        std::vector<std::vector<int>> histBinIdsCols(nDim);
        for (int iDim = 0; iDim < nDim; ++iDim) {
            auto& col = histBinIdsCols[iDim];
            col.resize(hist->dim()[iDim]);
            std::iota(col.begin(), col.end(), 0);
        }
        std::vector<std::vector<int>> histBinIdsArray =
                crossProduct(histBinIdsCols);

        for (std::vector<int> histBinIds : histBinIdsArray) {
            std::vector<int> histNBins = map<int, int>([hist](int iDim) {
                return hist->dim()[iDim];
            }, dims);
            std::vector<double> singleHistBinRanges =
                    map<std::tuple<Range, int>, double>(
                        [](const std::tuple<Range,int>& tuple) {
                return std::get<0>(tuple).range() / std::get<1>(tuple);
            }, zip(histRanges, histNBins));

            // std::cout << "singleHistBinRanges[0] = "
            //         << singleHistBinRanges[0] << std::endl;
            // std::cout << "singleHistBinRanges[1] = "
            //         << singleHistBinRanges[1] << std::endl;

            std::vector<Range> histBinRanges =
                    map<std::tuple<Range, int, double>, Range>(
                        [](const std::tuple<Range, int, double>& tuple) {
                double lower = std::get<0>(tuple).lower();
                int binDimId = std::get<1>(tuple);
                double singleBinDimRange = std::get<2>(tuple);
                return Range(lower + binDimId * singleBinDimRange,
                        lower + (binDimId + 1) * singleBinDimRange);
            }, zip(histRanges, histBinIds, singleHistBinRanges));
            double value = hist->bin(histBinIds).value();

            // std::cout << "histBinIds[0]" << histBinRanges[0] << std::endl;
            // std::cout << "histBinIds[1]" << histBinRanges[1] << std::endl;

            std::vector<int> begBinIds =
                    map<std::tuple<Range, Range, double>, int>(
                        [](const std::tuple<Range, Range, double>& tuple) {
                const auto& histBinRange = std::get<0>(tuple);
                const auto& range = std::get<1>(tuple);
                double singleBinRange = std::get<2>(tuple);
                return (histBinRange.lower() - range.lower()) / singleBinRange;
            }, zip(histBinRanges, ranges, singleBinRanges));
            std::vector<int> endBinIds =
                    map<std::tuple<Range, Range, double, int>, int>(
                        [](const std::tuple<Range, Range, double, int>& tuple) {
                const auto& histBinRange = std::get<0>(tuple);
                const auto& range = std::get<1>(tuple);
                double singleBinRange = std::get<2>(tuple);
                int nBin = std::get<3>(tuple);
                return calcEndBinId(histBinRange, range, singleBinRange, nBin);
            }, zip(histBinRanges, ranges, singleBinRanges, nBins));

            // std::cout << "begBinIds[0] = " << begBinIds[0]
            //         << " | endBinIds[0] = " << endBinIds[0] << std::endl;
            // std::cout << "begBinIds[1] = " << begBinIds[1]
            //         << " | endBinIds[1] = " << endBinIds[1] << std::endl;

            std::vector<std::vector<int>> binIdsCols(nDim);
            for (int iDim = 0; iDim < nDim; ++iDim) {
                auto& col = binIdsCols[iDim];
                col.resize(endBinIds[iDim] - begBinIds[iDim] + 1);
                std::iota(col.begin(), col.end(), begBinIds[iDim]);
            }
            std::vector<std::vector<int>> binIdsArray =
                    crossProduct(binIdsCols);
            for (std::vector<int> binIds : binIdsArray) {
                std::vector<Range> binRanges =
                        map<std::tuple<Range, int, double>, Range>(
                            [](const std::tuple<Range, int, double>& tuple) {
                    const Range& range = std::get<0>(tuple);
                    int binDimId = std::get<1>(tuple);
                    double singleBinRange = std::get<2>(tuple);
                    return Range(
                            range.lower() + binDimId * singleBinRange,
                            range.lower() + (binDimId + 1) * singleBinRange);
                }, zip(ranges, binIds, singleBinRanges));
                std::vector<double> portions =
                        map<std::tuple<Range, Range>, double>(
                            [](const std::tuple<Range, Range>& tuple) {
                    double range = Range::intersectRanges(
                            { std::get<0>(tuple), std::get<1>(tuple) }).range();
//                    double epsilon = std::numeric_limits<double>::epsilon();
//                    assert(range > -10.0 * epsilon);
                    return std::max(0.0, range);
                }, zip(binRanges, histBinRanges));
                double portionProduct = multiplyEach(portions);
                assert(portionProduct >= 0.0);
                double histBinRangeProduct = std::accumulate(
                        histBinRanges.begin(), histBinRanges.end(), 1.0,
                        [](double product, const Range& b) {
                    return product * b.range();
                });
                double ratio = portionProduct / histBinRangeProduct;
                assert(ratio >= 0.0);
                int iBin = Extent(nBins).idstoflat(binIds);

                // std::cout << "iBin = " << iBin << std::endl;

                values[iBin] += ratio * value;
            }
        }
    }
    // construct the new histogram
    std::vector<double> mins = map<Range, double>([](const Range& range) {
        return range.lower();
    }, ranges);
    std::vector<double> maxs = map<Range, double>([](const Range& range) {
        return range.upper();
    }, ranges);
    std::vector<double> logBases(nDim, 0.0);
    return std::shared_ptr<Hist>(
            Hist::fromDenseValues(
                nDim, nBins, mins, maxs, logBases, vars, values));
}

/*
std::shared_ptr<Hist> Hist1DMerger::merge(
        const std::vector<std::shared_ptr<const Hist>> &hists) const {
    // error checking
    std::vector<std::string> vars;
    for (auto hist : hists) {
        assert(1 == hist->nDim());
        if (vars.empty())
            vars = hist->vars();
        assert(vars == hist->vars());
    }
    // if same range and same bin widths, then merge the numbers only.

    // if different range or different bin widths, first calculate the new range
    // as the min and max, then calculates the new optimal bin widths according
    // to one of the methods, and finally put the old bins into new bins by
    // overlapping areas.
    int nBins = calcNBins(hists);
    // get new range
    Range range = Range::unionRanges(histsToRanges(0, hists));
    double singleBinRange = (range[1] - range[0]) / double(nBins);
    // put old values into new bins
    std::vector<double> values(nBins, 0.0);
    for (auto hist : hists) {
        double histRange = hist->dimMax(0) - hist->dimMin(0);
        for (int iBin = 0; iBin < hist->nBins(); ++iBin) {
            Range binRange(
                hist->dimMin(0) + double(iBin) / hist->nBins() * histRange,
                hist->dimMin(0) + double(iBin + 1) / hist->nBins() * histRange);
            double value = hist->bin(iBin).value();

            // std::cout << "binRange = [" << binRange[0] << ", " << binRange[1]
                    // << "]" << std::endl;
            // std::cout << "value = " << value << std::endl;

            int begBinId = (binRange[0] - range[0]) / singleBinRange;
            int endBinId = calcEndBinId(binRange, range, singleBinRange, nBins);

            // std::cout << "begBinId = " << begBinId << " endBinId = "
            //         << endBinId << std::endl;

            if (begBinId == endBinId) {
                values[begBinId] += value;
            } else {
                double begBinPortion = range[0]
                        + (begBinId + 1) * singleBinRange - binRange[0];
                double endBinPortion =
                        binRange[1] - endBinId * singleBinRange - range[0];
                double midPortion = (endBinId - begBinId - 1) * singleBinRange;
                double totalPortion =
                        begBinPortion + endBinPortion + midPortion;

                // std::cout << "begBinPortion = " << begBinPortion
                //         << " endBinPortion = " << endBinPortion
                //         << " midPortion = " << midPortion << std::endl;

                values[begBinId] += value * begBinPortion / totalPortion;
                values[endBinId] += value * endBinPortion / totalPortion;
                for (int binId = begBinId + 1; binId < endBinId; ++binId) {
                    values[binId] += value * singleBinRange / totalPortion;
                }
            }
        }
    }

    // std::cout << "nBins = " << nBins << std::endl;
    // std::cout << "range = [" << range[0] << ", " << range[1] << "]"
    //         << std::endl;
    // std::cout << "singleBinRange = " << singleBinRange << std::endl;

    // construct the new histogram
    return std::make_shared<Hist1D>(
            nBins, range[0], range[1], 0.0, vars[0], values);
}
*/
