#include <cassert>
#include <cmath>
#include <data/Histogram.h>

int main(void)
{
	std::vector<double> values = {0.0, 1.0, 0.0, 2.0, 0.0, 0.0, 0.0, 3.0};
	auto three = std::make_shared<Hist3DFull>(2, 2, 2, values);
	assert(three->checkRange({{1, 1}, {1, 1}, {1, 1}}, 49.f));
	assert(!three->checkRange({{1, 1}, {1, 1}, {1, 1}}, 51.f));
	std::cout << three->binSum({{1, 1}, {1, 1}, {1, 1}}).value() << std::endl;
	assert(fabs(three->binSum({{1, 1}, {1, 1}, {1, 1}}).value() - 3.0) < 0.0001);
	std::cout << three->binSum({{1, 1}, {1, 1}, {0, 1}}).value() << std::endl;
	assert(fabs(three->binSum({{1, 1}, {1, 1}, {0, 1}}).value() - 5.0) < 0.0001);
	std::cout << three->binSum({{0, 1}, {0, 1}, {0, 1}}).value() << std::endl;
	assert(fabs(three->binSum({{0, 1}, {0, 1}, {0, 1}}).value() - 6.0) < 0.0001);
	// 2D
	auto two = std::make_shared<Hist2D>(4, 2, values);
	assert(fabs(two->binSum({{3, 3}, {1, 1}}).value() - 3.0) < 0.0001);
	assert(fabs(two->binSum({{3, 3}, {0, 1}}).value() - 5.0) < 0.0001);
	// more
	std::vector<double> more = values;
	more.insert(more.end(), values.begin(), values.end());
	auto eight = std::make_shared<Hist2D>(8, 2, more);
	assert(fabs(two->binSum({{0, 3}, {0, 0}}).value() - 3.0) < 0.0001);
	std::cout << two->binSum({{4, 7}, {0, 0}}).value() << std::endl;
	assert(fabs(two->binSum({{4, 7}, {0, 0}}).value() - 3.0) < 0.0001);

	return 0;
}
