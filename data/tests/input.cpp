#include <cassert>
#include <cmath>
#include <DataPool.h>

int main(void)
{
	// DataPool
	auto pool = std::make_shared<DataPool>();
	pool->setDir("./s3d_run/");
	std::vector<std::shared_ptr<DataStep> > steps(3);
	for (unsigned int iStep = 0; iStep < steps.size(); ++iStep)
		steps[iStep] = pool->step(iStep);
	assert(steps[0]);
	assert(steps[1]);
	assert(!steps[2]);

	// DataStep
	auto volume = pool->step(1)->volume("vx-vy-vz");
	assert(volume);
	assert(!pool->step(0)->volume("lksdjfs"));

	// Helper
	assert(volume->domain(0)->helper().N_HIST == 64);

	// Histogram
	auto hist = volume->domain(0)->hist(16);
	assert(hist);

	auto bin = hist->bin(1, 1, 1);
	assert(fabs(bin.value()) < 0.0001);

	return 0;
}