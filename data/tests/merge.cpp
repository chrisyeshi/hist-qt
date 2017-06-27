#include <cassert>
#include <cmath>
#include <memory>
#include <iostream>
#include <Histogram.h>
#include <histmerger.h>

int main(void)
{
    {
        std::vector<std::shared_ptr<const Hist>> hists(2);
        hists[0] = std::make_shared<Hist1D>(1, 0.0, 0.8, 0.0, "test",
                std::vector<double>{0.9});
        hists[1] = std::make_shared<Hist1D>(1, 0.2, 1.0, 0.0, "test",
                std::vector<double>{0.9});
        std::shared_ptr<Hist> merged = Hist1DMerger().merge(hists);
        assert(fabs(merged->values()[0] - merged->values()[1]) < 0.0001);
        auto values = merged->values();
        for (auto value : values) {
            std::cout << value << ", ";
        }
        std::cout << std::endl;
    }
    {
        std::vector<std::shared_ptr<const Hist>> hists(2);
        hists[0] = std::make_shared<Hist1D>(1, 1.0, 1.8, 0.0, "test",
                std::vector<double>{0.9});
        hists[1] = std::make_shared<Hist1D>(1, 1.2, 2.0, 0.0, "test",
                std::vector<double>{0.9});
        std::shared_ptr<Hist> merged = Hist1DMerger().merge(hists);
        assert(fabs(merged->values()[0] - merged->values()[1]) < 0.0001);
        auto values = merged->values();
        for (auto value : values) {
            std::cout << value << ", ";
        }
        std::cout << std::endl;
    }

    

    return 0;
}
