#include "../SimpleGame/VillageModels.h"
#include "../SimpleGame/AssetCache.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <cstdio>

int main()
{
	const uint64_t key = VillageModels::Key({1, 12, 30, .8f, 0});
	assert(key == VillageModels::Key({1, 12, 30, .8f, 0}));
	assert(key != VillageModels::Key({1, 12, 30, 1, 0}));
	VillageModels cache;
	VillageModels::Parts parts;
	parts[0] = {{0, 0, 1, .5f, .25f, 1, 0, 0, 0},
	            {1, 0, 1, .5f, .25f, 1, 0, 0, 0},
	            {0, 1, 1, .5f, .25f, 1, 0, 0, 0}};
	parts[1] = parts[0];
	cache.Store(key, parts);
	cache.Save();
	VillageModels loaded;
	const auto* reloaded = loaded.Find(key);
	assert(reloaded && (*reloaded)[0].size() == 3 && (*reloaded)[1].size() == 3);
	assert(std::memcmp((*reloaded)[0].data(), parts[0].data(), 3 * sizeof(ModelVertex)) == 0);
	const auto* stable = reloaded;
	loaded.Store(VillageModels::Key({0, 3, 2}), parts);
	assert(loaded.Find(key) == stable);
	std::vector<unsigned char> invalid = {0xff, 0xff, 0xff, 0x7f};
	assert(SaveAssetCache(L"village-models-v1.mesh", 1, invalid));
	VillageModels corrupted;
	assert(!corrupted.Find(key));
	corrupted.Store(key, parts);
	corrupted.Save();
	VillageModels repaired;
	assert(repaired.Find(key));
	std::vector<unsigned char> data;
	assert(LoadAssetCache(L"village-models-v1.mesh", 1, 128 * 1024 * 1024, data));
	// Reject nonfinite geometry even when the outer cache checksum is valid.
	float nan = std::numeric_limits<float>::quiet_NaN();
	std::memcpy(
	    data.data() + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t), &nan, sizeof(nan));
	assert(SaveAssetCache(L"village-models-v1.mesh", 1, data));
	VillageModels badGeometry;
	assert(!badGeometry.Find(key));
	badGeometry.Store(key, parts);
	badGeometry.Save();
	puts(
	    "PASS: village mesh round trip, appearance keys, stable references, malformed cache recovery and geometry validation.");
}
