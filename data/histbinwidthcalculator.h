#ifndef HISTBINWIDTHCALCULATOR_H
#define HISTBINWIDTHCALCULATOR_H

#include <cmath>

class HistBinWidthCalculatorSturges {
public:
    static int nBins(double totalValue) {
        return ceil(log2(totalValue)) + 1;
    }
};

#endif // HISTBINWIDTHCALCULATOR_H