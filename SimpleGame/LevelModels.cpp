#include "stdafx.h"
#include "LevelModels.h"
#include "AssetCache.h"
#include <cmath>
#include <cstring>
#include <iostream>

namespace
{
	static_assert(sizeof(ModelVertex) == 9 * sizeof(float),
	              "Update the model cache format when vertex layout changes.");

	void Polygon(std::vector<ModelVertex>& model,
	             const std::vector<Point>& points,
	             Color color,
	             Material material = Material::Plain)
	{
		for (size_t i = 1; i + 1 < points.size(); ++i)
			for (Point p : {points[0], points[i], points[i + 1]})
				model.push_back({p.x,
				                 p.y,
				                 color.r,
				                 color.g,
				                 color.b,
				                 color.a,
				                 -static_cast<float>(material) - 1,
				                 p.x,
				                 -p.y});
	}

	void Oval(std::vector<ModelVertex>& model,
	          float x,
	          float y,
	          float rx,
	          float ry,
	          Color c,
	          Material material = Material::Plain)
	{
		std::vector<Point> points;
		for (int i = 0; i < 24; ++i)
		{
			float angle = i * 6.2831853f / 24;
			points.push_back({x + std::cos(angle) * rx, y + std::sin(angle) * ry});
		}
		Polygon(model, points, c, material);
	}
}

const std::vector<ModelVertex>& LevelModels::Get(LevelModel model) const
{
	return m_Models[static_cast<size_t>(model)];
}

void LevelModels::Load()
{
	if (!m_Models[0].empty())
		return;
	std::vector<unsigned char> data;
	bool valid = LoadAssetCache(L"level-models-v2.mesh", 2, 4 * 1024 * 1024, data);
	size_t offset = 0;
	for (auto& model : m_Models)
	{
		uint32_t count = 0;
		if (!valid || offset + sizeof(count) > data.size())
		{
			valid = false;
			break;
		}
		std::memcpy(&count, data.data() + offset, sizeof(count));
		offset += sizeof(count);
		if (!count || count > 30000 || count % 3 ||
		    offset + count * sizeof(ModelVertex) > data.size())
		{
			valid = false;
			break;
		}
		model.resize(count);
		std::memcpy(model.data(), data.data() + offset, count * sizeof(ModelVertex));
		offset += count * sizeof(ModelVertex);
		for (const auto& vertex : model)
		{
			for (float value : {vertex.x,
			                    vertex.y,
			                    vertex.r,
			                    vertex.g,
			                    vertex.b,
			                    vertex.a,
			                    vertex.material,
			                    vertex.u,
			                    vertex.v})
				if (!std::isfinite(value) || std::abs(value) > 512)
					valid = false;
			if (vertex.material > -1 || vertex.material < -12 ||
			    vertex.material != std::floor(vertex.material))
				valid = false;
		}
	}
	if (valid && offset == data.size())
	{
		std::cout << "Level models loaded from cache.\n";
		return;
	}
	for (auto& model : m_Models)
		model.clear();
	auto& tree = m_Models[0];
	Polygon(tree, {{-5, 0}, {5, 0}, {4, -76}, {-3, -76}}, {.29f, .20f, .12f, 1});
	for (int i = 0; i < 12; ++i)
	{
		float a = i * 2.4f;
		Oval(tree,
		     std::cos(a) * 25,
		     -73 + std::sin(a) * 25,
		     20,
		     24,
		     {.22f + (i % 3) * .03f, .36f + (i % 3) * .025f, .18f, 1},
		     Material::Foliage);
	}
	auto& rock = m_Models[1];
	Polygon(rock,
	        {{-28, -5}, {-20, -28}, {9, -36}, {29, -17}, {24, 3}, {0, 10}},
	        {.32f, .36f, .33f, 1});
	Polygon(rock, {{-20, -28}, {9, -36}, {18, -21}, {-3, -10}, {-28, -5}}, {.49f, .51f, .43f, 1});
	auto& slime = m_Models[2];
	Oval(slime, 0, -12, 22, 15, {.22f, .49f, .31f, 1});
	Oval(slime, -4, -20, 14, 13, {.39f, .66f, .38f, 1});
	Oval(slime, -8, -25, 5, 3, {.66f, .82f, .46f, 1});
	for (int side : {-1, 1})
		Oval(slime, side * 7.f, -17, 2.5f, 4, {.035f, .08f, .05f, 1});
	auto& loot = m_Models[3];
	Oval(loot, 0, -4, 9, 6, {.50f, .31f, .13f, 1});
	Polygon(loot, {{-4, -9}, {0, -21}, {6, -12}, {0, -5}}, {.55f, 1.25f, 1.12f, 1});
	Oval(loot, -7, -4, 4, 2, {.96f, .70f, .22f, 1});
	auto& camp = m_Models[4];
	Oval(camp, 0, 0, 25, 12, {.35f, .34f, .27f, 1});
	Polygon(camp, {{-20, 0}, {-17, -6}, {18, -1}, {20, 6}}, {.35f, .20f, .09f, 1});
	// Flame geometry stays fixed; the fragment shader owns its silhouette and motion.
	size_t flame = camp.size();
	Polygon(camp, {{-24, 4}, {24, 4}, {24, -68}, {-24, -68}}, {1, 1, 1, 1}, Material::Flame);
	for (size_t i = flame; i < camp.size(); ++i)
	{
		camp[i].u = camp[i].x / 24;
		camp[i].v = (4 - camp[i].y) / 72;
	}

	auto& wolf = m_Models[static_cast<size_t>(LevelModel::Wolf)];
	Oval(wolf, -2, -20, 25, 12, {.38f, .40f, .36f, 1});
	Polygon(wolf,
	        {{13, -24}, {24, -38}, {32, -27}, {43, -21}, {30, -13}, {13, -14}},
	        {.49f, .51f, .45f, 1});
	Polygon(wolf, {{20, -34}, {19, -47}, {28, -36}}, {.29f, .31f, .28f, 1});
	Polygon(wolf, {{-23, -23}, {-42, -35}, {-34, -19}, {-21, -13}}, {.32f, .34f, .30f, 1});
	for (int side : {-1, 1})
		Polygon(wolf,
		        {{side * 17.f - 3, -17},
		         {side * 17.f + 4, -17},
		         {side * 17.f + 5, 0},
		         {side * 17.f - 2, 0}},
		        {.25f, .27f, .24f, 1});
	Oval(wolf, 30, -28, 2, 2, {1.2f, .78f, .22f, 1});

	auto& beetle = m_Models[static_cast<size_t>(LevelModel::Beetle)];
	for (int side : {-1, 1})
		for (int leg = 0; leg < 3; ++leg)
			Polygon(beetle,
			        {{side * 12.f, -26.f + leg * 9},
			         {side * 30.f, -17.f + leg * 7},
			         {side * 28.f, -12.f + leg * 7},
			         {side * 10.f, -21.f + leg * 9}},
			        {.30f, .23f, .14f, 1});
	Oval(beetle, 0, -19, 20, 21, {.34f, .30f, .21f, 1}, Material::Stone);
	Oval(beetle, -5, -25, 11, 14, {.59f, .48f, .27f, 1}, Material::Stone);
	Polygon(beetle, {{-2, -38}, {2, -38}, {2, -3}, {-2, -3}}, {.17f, .17f, .13f, 1});
	Oval(beetle, 0, -4, 12, 7, {.22f, .20f, .13f, 1});

	auto& guardian = m_Models[static_cast<size_t>(LevelModel::Guardian)];
	const Color stone = {.45f, .48f, .46f, 1};
	for (int side : {-1, 1})
	{
		Polygon(guardian,
		        {{side * 9.f - 7, -18},
		         {side * 9.f + 6, -18},
		         {side * 9.f + 6, 1},
		         {side * 9.f - 7, 1}},
		        stone,
		        Material::Stone);
		Polygon(guardian,
		        {{side * 27.f - 8, -57},
		         {side * 27.f + 8, -57},
		         {side * 27.f + 9, -20},
		         {side * 27.f - 9, -20}},
		        stone,
		        Material::Stone);
	}
	Polygon(guardian,
	        {{-20, -60}, {20, -60}, {17, -20}, {-17, -20}},
	        {.36f, .40f, .39f, 1},
	        Material::Stone);
	Polygon(guardian, {{-13, -79}, {13, -79}, {13, -58}, {-13, -58}}, stone, Material::Stone);
	Polygon(guardian, {{-7, -47}, {0, -56}, {7, -47}, {0, -35}}, {.45f, 1.15f, 1.2f, 1});
	for (int side : {-1, 1})
		Oval(guardian, side * 6.f, -68, 3, 2, {.5f, 1.4f, 1.5f, 1});

	auto& dwarf = m_Models[static_cast<size_t>(LevelModel::Dwarf)];
	for (int side : {-1, 1})
		Oval(dwarf, side * 10.f, -3, 8, 6, {.24f, .18f, .12f, 1});
	Oval(dwarf, 0, -24, 20, 21, {.34f, .42f, .46f, 1});
	Oval(dwarf, 0, -48, 14, 13, {.77f, .58f, .36f, 1});
	Polygon(dwarf, {{-14, -45}, {14, -45}, {9, -20}, {0, -15}, {-10, -24}}, {.64f, .43f, .22f, 1});
	Oval(dwarf, 0, -58, 18, 7, {.43f, .48f, .48f, 1}, Material::Stone);
	Polygon(dwarf, {{-21, -31}, {-16, -31}, {-16, 0}, {-21, 0}}, {.40f, .27f, .13f, 1});
	Polygon(dwarf, {{-30, -35}, {-9, -35}, {-6, -28}, {-27, -28}}, {.65f, .67f, .60f, 1});

	auto& archive = m_Models[static_cast<size_t>(LevelModel::Archive)];
	Polygon(archive,
	        {{-23, 0}, {23, 0}, {23, -35}, {-23, -35}},
	        {.44f, .43f, .35f, 1},
	        Material::Stone);
	Polygon(archive, {{-20, -36}, {12, -45}, {27, -33}, {-7, -24}}, {.80f, .73f, .53f, 1});
	for (int line = 0; line < 4; ++line)
		Polygon(archive,
		        {{-12.f + line * 3, -35},
		         {9.f + line * 3, -40},
		         {10.f + line * 3, -38},
		         {-11.f + line * 3, -33}},
		        {.34f, .35f, .29f, 1});

	auto& hall = m_Models[static_cast<size_t>(LevelModel::DwarfHall)];
	for (int side : {-1, 1})
		Polygon(hall,
		        {{side * 35.f - 10, 0},
		         {side * 35.f + 10, 0},
		         {side * 35.f + 10, -95},
		         {side * 35.f - 10, -95}},
		        stone,
		        Material::Stone);
	Polygon(hall,
	        {{-52, -97}, {-27, -120}, {29, -120}, {52, -97}},
	        {.52f, .54f, .47f, 1},
	        Material::Stone);
	Polygon(hall, {{-10, -106}, {0, -117}, {10, -106}, {0, -97}}, {.55f, 1.18f, 1.1f, 1});

	auto& trailLoot = m_Models[static_cast<size_t>(LevelModel::TrailLoot)];
	Oval(trailLoot, 0, -7, 13, 9, {.55f, .33f, .16f, 1});
	Polygon(trailLoot, {{-7, -17}, {7, -17}, {4, -10}, {-4, -10}}, {.69f, .55f, .31f, 1});
	auto& oreLoot = m_Models[static_cast<size_t>(LevelModel::OreLoot)];
	Polygon(oreLoot,
	        {{-13, 0}, {-8, -18}, {4, -26}, {15, -10}, {9, 3}},
	        {.55f, .46f, .28f, 1},
	        Material::Stone);
	Polygon(oreLoot, {{-8, -18}, {4, -26}, {6, -9}, {-3, -2}}, {.91f, .71f, .35f, 1});
	auto& partsLoot = m_Models[static_cast<size_t>(LevelModel::PartsLoot)];
	Oval(partsLoot, 0, -8, 14, 10, {.42f, .50f, .51f, 1});
	Oval(partsLoot, 0, -8, 6, 5, {.18f, .23f, .23f, 1});
	for (int side : {-1, 1})
		Polygon(partsLoot,
		        {{side * 12.f - 4, -18},
		         {side * 12.f + 4, -18},
		         {side * 12.f + 4, -1},
		         {side * 12.f - 4, -1}},
		        {.57f, .62f, .59f, 1});

	data.clear();
	for (const auto& model : m_Models)
	{
		uint32_t count = static_cast<uint32_t>(model.size());
		size_t start = data.size();
		data.resize(start + sizeof(count) + count * sizeof(ModelVertex));
		std::memcpy(data.data() + start, &count, sizeof(count));
		std::memcpy(data.data() + start + sizeof(count), model.data(), count * sizeof(ModelVertex));
	}
	SaveAssetCache(L"level-models-v2.mesh", 2, data);
	std::cout << "Level models generated and cache save attempted.\n";
}
