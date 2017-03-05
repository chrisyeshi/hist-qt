#include <cassert>
#include <cmath>
#include <memory>
#include <data/Histogram.h>

int main(void)
{
    std::vector<double> values = {0.0, 1.0, 0.0, 2.0, 0.0, 0.0, 0.0, 3.0};
    int dimx = 2, dimy = 2, dimz = 3;
    std::vector<double> mins = {0.0, 0.0, 0.0};
    std::vector<double> maxs = {5.0, 5.0, 5.0};
    std::vector<double> logbases = {1.0, 1.0, 1.0};
    std::vector<std::string> vars = {"x", "y", "z"};
    auto three = std::make_shared<Hist3DFull>(
            dimx, dimy, dimz, mins, maxs, logbases, vars, values);
    assert(fabs(three->to2D(0, 1).bin(3).value() - 5.0) < 0.0001);
    assert(fabs(three->to2D(0, 2).bin(1).value() - 3.0) < 0.0001);

	return 0;
}
