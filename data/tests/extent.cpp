#include <iostream>
#include <cassert>
#include <cmath>
#include <Extent.h>

int main(void)
{
	std::vector<int> id2, id3;
	int flatId, x, y, z;

	// 2D
	std::vector<int> dim2 = {5, 6};
	flatId = Extent(dim2).idstoflat({2, 2});
	assert(flatId == 12);
	flatId = Extent(dim2).idstoflat(2, 2);
	assert(flatId == 12);
	id2 = Extent(dim2).flattoids(12);
	assert(id2[0] == 2 && id2[1] == 2);
        Extent(dim2).flattoids(12, &x, &y);
        std::cout << x << " " << y << std::endl;
        assert(x == 2 && y == 2);

	// 3D
	std::vector<int> dim3 = {3, 4, 5};
	flatId = Extent(dim3).idstoflat({2, 3, 4});
	assert(flatId == 2 + 9 + 3 * 4 * 4);
	flatId = Extent(dim3).idstoflat(2, 3, 4);
	assert(flatId == 59);
	id3 = Extent(dim3).flattoids(59);
	assert(id3[0] == 2 && id3[1] == 3 && id3[2] == 4);
        Extent(dim3).flattoids(59, &x, &y, &z);
        assert(x == 2 && y == 3 && z == 4);

	// parameter pack constructor
	assert(Extent(4, 5).idstoflat(3, 4) == 19);
	assert(Extent(4, 5, 6).idstoflat(1, 2, 3) == 69);
	assert(Extent(10).nDim() == 10);
	assert(Extent(1, 2).nDim() == 2);

	return 0;
}
