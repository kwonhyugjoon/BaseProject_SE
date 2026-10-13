#include "../SimpleGame/TreeCollisionGrid.h"
#include <cassert>
#include <chrono>
#include <cstdio>

int main()
{
	std::vector<Point> trees, queries;
	TreeCollisionGrid grid(152, 132);
	for (int i = 0; i < 1100; ++i)
	{
		Point p = {3.f + (i * 37) % 146, 3.f + (i * 23) % 126};
		trees.push_back(p);
		grid.Add(p);
	}
	for (int i = 0; i < 50000; ++i)
		queries.push_back({(i * 71 % 15200) * .01f, (i * 113 % 13200) * .01f});
	auto linear = [&](Point p)
	{
		for (Point tree : trees)
			if (std::hypot(p.x - tree.x, p.y - tree.y) < .6f)
				return true;
		return false;
	};
	for (Point p : queries)
		assert(linear(p) == grid.Blocks(p));
	auto measure = [&](bool indexed)
	{
		auto start = std::chrono::steady_clock::now();
		int hits = 0;
		for (Point p : queries)
			hits += indexed ? grid.Blocks(p) : linear(p);
		double ms =
		    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
		        .count();
		printf("%s: %.3f ms, hits=%d\n", indexed ? "grid" : "linear", ms, hits);
	};
	measure(false);
	measure(true);
	grid.Clear();
	assert(!grid.Blocks(trees.front()));
	grid.Add({8, 8});
	assert(grid.Blocks({7.9f, 7.9f}));
	assert(!grid.Blocks({7, 7}));
	grid.Clear();
	grid.Add({30, 30});
	assert(!grid.Blocks({8, 8}) && grid.Blocks({30, 30}));
	puts("50000 equivalence queries, cell boundary and rebuild tests passed.");
}
