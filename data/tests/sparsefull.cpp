#include <cassert>
#include <cmath>
#include <Histogram.h>

int main(void)
{
	std::vector<double> values = {0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 8.0};
	auto full = std::make_shared<Hist3DFull>(2, 2, 2, values);
	auto sparse = full->toSparse();
	assert(fabs(sparse->bin(1).value() - 2.0) < 0.0001);
	assert(fabs(sparse->bin(4).value() - 0.0) < 0.0001);
	assert(fabs(sparse->bin(7).value() - 8.0) < 0.0001);
	auto newfull = sparse->toFull();
	assert(fabs(newfull->bin(1).value() - 2.0) < 0.0001);
	assert(fabs(newfull->bin(4).value() - 0.0) < 0.0001);
	assert(fabs(newfull->bin(7).value() - 8.0) < 0.0001);

	return 0;
}