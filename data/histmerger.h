#ifndef HISTMERGER_H
#define HISTMERGER_H

#include <memory>
#include "Histogram.h"

class BinCount {
public:
    BinCount() : _binCount(-1), _methodName("sturges") {}
    BinCount(int binCount) : _binCount(binCount) {}
    BinCount(std::string methodName) : _binCount(-1), _methodName(methodName) {}

public:
    int _binCount;
    std::string _methodName;
};

class HistMerger {
public:
    HistMerger() {}
    HistMerger(std::vector<BinCount> binCounts) : _binCounts(binCounts) {}

public:
    virtual std::shared_ptr<Hist> merge(
            const std::vector<std::shared_ptr<const Hist>>& hists) const;

protected:
    int calcBinCount(int iDim,
            const std::vector<std::shared_ptr<const Hist>>& hists) const;
    std::vector<int> calcBinCounts(
            const std::vector<std::shared_ptr<const Hist>>& hists) const;

private:
    std::vector<BinCount> _binCounts;
};

#endif // HISTMERGER_H
