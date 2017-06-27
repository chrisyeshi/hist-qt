#include "histmerger.h"
#include <cmath>
#include <limits>
#include <vector>
#include <array>

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
    double range[2] = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest()
    };
    for (auto hist : hists) {
        range[0] = std::min(hist->dimMin(0), range[0]);
        range[1] = std::max(hist->dimMax(0), range[1]);
    }
    double singleBinRange = (range[1] - range[0]) / double(nBins);
    // put old values into new bins
    std::vector<double> values(nBins, 0.0);
    for (auto hist : hists) {
        double histRange = hist->dimMax(0) - hist->dimMin(0);
        for (int iBin = 0; iBin < hist->nBins(); ++iBin) {
            std::array<double, 2> binRange = {
                hist->dimMin(0) + double(iBin) / hist->nBins() * histRange,
                hist->dimMin(0) + double(iBin + 1) / hist->nBins() * histRange
            };
            double value = hist->bin(iBin).value();

            // std::cout << "binRange = [" << binRange[0] << ", " << binRange[1]
                    // << "]" << std::endl;
            // std::cout << "value = " << value << std::endl;

            int begBinId = (binRange[0] - range[0]) / singleBinRange;
            int endBinId = (binRange[1] - range[0]) / singleBinRange;

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

int Hist1DMerger::calcNBins(
        const std::vector<std::shared_ptr<const Hist>> &hists) const {
    if (_nBins > 0) {
        return _nBins;
    }
    /// TODO: extract these into HistBinWidthCalculator?
    if ("sturges" == _methodName) {
        double totalValue = 0.0;
        for (auto hist : hists) {
            totalValue += hist->binSum().value();
        }
        // apply sturge's formula
        return log2(totalValue) + 1;
    }
    throw "Incorrect nBins and/or method names.";
}
