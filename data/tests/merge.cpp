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
        std::shared_ptr<Hist> merged = HistMerger({2}).merge(hists);
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
        std::shared_ptr<Hist> merged = HistMerger({2}).merge(hists);
        assert(fabs(merged->values()[0] - merged->values()[1]) < 0.0001);
        auto values = merged->values();
        for (auto value : values) {
            std::cout << value << ", ";
        }
        std::cout << std::endl;
    }
    {
        std::vector<std::shared_ptr<const Hist>> hists(2);
        hists[0] = std::make_shared<Hist2D>(
                1, 1,
                std::vector<double>{ 1.0, 1.0 },
                std::vector<double>{ 1.8, 1.8 },
                std::vector<double>{ 0.0, 0.0 },
                std::vector<std::string>{ "a", "b" },
                std::vector<double>{ 0.9 });
        hists[1] = std::make_shared<Hist2D>(
                1, 1,
                std::vector<double>{ 1.2, 1.2 },
                std::vector<double>{ 2.0, 2.0 },
                std::vector<double>{ 0.0, 0.0 },
                std::vector<std::string>{ "a", "b" },
                std::vector<double>{ 0.9 });
        std::shared_ptr<Hist> merged = HistMerger({2, 2}).merge(hists);
        assert(fabs(merged->values()[0] - merged->values()[3]) < 0.0001);
        assert(fabs(merged->values()[1] - merged->values()[2]) < 0.0001);
        auto values = merged->values();
        for (auto value : values) {
            std::cout << value << ", ";
        }
        std::cout << std::endl;
    }
    {
        std::vector<std::shared_ptr<const Hist>> hists(2);
        hists[0] = std::make_shared<Hist2D>(
                2, 1,
                std::vector<double>{ 1.0, 1.2 },
                std::vector<double>{ 2.6, 1.6 },
                std::vector<double>{ 0.0, 0.0 },
                std::vector<std::string>{ "a", "b" },
                std::vector<double>{ 0.9, 0.9 });
        hists[1] = std::make_shared<Hist2D>(
                1, 2,
                std::vector<double>{ 1.2, 1.0 },
                std::vector<double>{ 1.6, 2.6 },
                std::vector<double>{ 0.0, 0.0 },
                std::vector<std::string>{ "a", "b" },
                std::vector<double>{ 0.9, 0.9 });
        std::shared_ptr<Hist> merged = HistMerger({2, 2}).merge(hists);
        auto values = merged->values();
        for (auto value : values) {
            std::cout << value << ", ";
        }
        std::cout << std::endl;
        assert(fabs(merged->values()[0]
                - (merged->values()[1] + merged->values()[2])) < 0.0001);
        assert(fabs(merged->values()[1] - merged->values()[2]) < 0.0001);
        assert(fabs(merged->values()[3]) < 0.0001);
    }
    {
        // binCounts out of bound
        std::vector<std::shared_ptr<const Hist>> hists(2);
        hists[0] = std::make_shared<Hist1D>(1, 0.0, 0.8, 0.0, "test",
                std::vector<double>{0.9});
        hists[1] = std::make_shared<Hist1D>(1, 0.2, 1.0, 0.0, "test",
                std::vector<double>{0.9});
        std::shared_ptr<Hist> merged = HistMerger().merge(hists);
        auto values = merged->values();
        for (auto value : values) {
            std::cout << value << ", ";
        }
        std::cout << std::endl;
    }
    {
        // freedman
        std::vector<std::shared_ptr<const Hist>> hists(2);
        hists[0] = std::make_shared<Hist1D>(3, 0.0, 0.8, 0.0, "test",
                std::vector<double>{12, 24, 36});
        hists[1] = std::make_shared<Hist1D>(3, 0.2, 1.0, 0.0, "test",
                std::vector<double>{36, 24, 12});
        std::shared_ptr<Hist> merged =
                HistMerger({BinCount("freedman")}).merge(hists);
        assert(merged->values().size() == 5);
        assert(fabs(merged->values()[0] - 9.0) < 0.0001);
        assert(fabs(merged->values()[1] - 42.0) < 0.0001);
        assert(fabs(merged->values()[2] - 42.0) < 0.0001);
        assert(fabs(merged->values()[3] - 42.0) < 0.0001);
        assert(fabs(merged->values()[4] - 9.0) < 0.0001);
        auto values = merged->values();
        for (auto value : values) {
            std::cout << value << ", ";
        }
        std::cout << std::endl;
    }

    return 0;
}
