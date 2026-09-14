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
	bool valid = LoadAssetCache(L"level-models-v1.mesh", 1, 4 * 1024 * 1024, data);
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
	data.clear();
	for (const auto& model : m_Models)
	{
		uint32_t count = static_cast<uint32_t>(model.size());
		size_t start = data.size();
		data.resize(start + sizeof(count) + count * sizeof(ModelVertex));
		std::memcpy(data.data() + start, &count, sizeof(count));
		std::memcpy(data.data() + start + sizeof(count), model.data(), count * sizeof(ModelVertex));
	}
	SaveAssetCache(L"level-models-v1.mesh", 1, data);
	std::cout << "Level models generated and cache save attempted.\n";
}
