#ifndef HISTMERGER_H
#define HISTMERGER_H

#include <memory>
#include "Histogram.h"

class HistMerger {
public:
    virtual std::shared_ptr<Hist> merge(
            const std::vector<std::shared_ptr<const Hist>>& hists) const = 0;
};

class Hist1DMerger : public HistMerger {
public:
    Hist1DMerger() {}
    Hist1DMerger(int nBins) : _nBins(nBins) {}
    Hist1DMerger(std::string methodName) : _methodName(methodName) {}

public:
    virtual std::shared_ptr<Hist> merge(
            const std::vector<std::shared_ptr<const Hist>> &hists)
                const override;

private:
    int calcNBins(const std::vector<std::shared_ptr<const Hist>> &hists) const;

private:
    int _nBins = -1;
    std::string _methodName = "sturges";
};

#endif // HISTMERGER_H
