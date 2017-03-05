#include <cassert>
#include <cmath>
#include <Histogram.h>

int main(void)
{
	{
		std::vector<int> dimDomains = {2, 2, 2};
		std::shared_ptr<HistVolume> volume = std::make_shared<HistVolume>("./s3d_run/data/tracer-1.0000E-07/", "vx-vy-vz", dimDomains);
		assert(volume);
		// helper
		assert(volume->helper().N_HIST == 64 * 8);
		assert(volume->helper().nh_x == 8);
		assert(volume->helper().nh_y == 8);
		assert(volume->helper().nh_z == 8);
		// access
		assert(volume->hist(4));
		assert(volume->domain(1)->hist(0));
		assert(volume->hist(4) == volume->domain(1)->hist(0));
		assert(volume->hist({5, 5, 5}) == volume->domain(7)->hist({1, 1, 1}));
		assert(volume->hist({5, 5, 5}) == volume->hist(365));
		assert(volume->hist(3, 4, 5) == volume->hist({3, 4, 5}));
	}

	{
		std::vector<int> dimDomains = {3, 3, 3};
		std::shared_ptr<HistVolume> volume = std::make_shared<HistVolume>("./s3d_run/data/333/", "vx-vy-vz", dimDomains);
		assert(volume);
		// domain
		assert(volume->nDomains() == 27);
		assert(volume->domain(0)->helper().N_HIST == 8);
		// helper
		std::cout << volume->helper().N_HIST << std::endl;
		assert(volume->helper().N_HIST == 3 * 3 * 3 * 8);
		assert(volume->helper().nh_x == 3 * 2);
		assert(volume->helper().nh_y == 3 * 2);
		assert(volume->helper().nh_z == 3 * 2);
	}

	return 0;
}