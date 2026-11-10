#include "stdafx.h"
#include "VillageModels.h"
#include "AssetCache.h"
#include <cmath>
#include <cstring>

namespace
{
	constexpr size_t MaximumBytes = 128 * 1024 * 1024;
	constexpr uint32_t MaximumModels = 8192;

	template <typename T> void Append(std::vector<unsigned char>& data, const T& value)
	{
		const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
		data.insert(data.end(), bytes, bytes + sizeof(T));
	}

	template <typename T>
	bool Read(const std::vector<unsigned char>& data, size_t& offset, T& value)
	{
		if (offset + sizeof(T) > data.size())
			return false;
		std::memcpy(&value, data.data() + offset, sizeof(T));
		offset += sizeof(T);
		return true;
	}
}

uint64_t VillageModels::Key(std::initializer_list<float> parameters)
{
	uint64_t hash = 14695981039346656037ull;
	for (float parameter : parameters)
	{
		uint32_t bits;
		std::memcpy(&bits, &parameter, sizeof(bits));
		for (int i = 0; i < 4; ++i)
			hash = (hash ^ ((bits >> (i * 8)) & 255)) * 1099511628211ull;
	}
	return hash;
}

void VillageModels::Load()
{
	if (m_Loaded)
		return;
	m_Loaded = true;
	std::vector<unsigned char> data;
	if (!LoadAssetCache(L"village-models-v1.mesh", 1, MaximumBytes, data))
		return;
	size_t offset = 0;
	uint32_t count = 0;
	bool valid = Read(data, offset, count) && count <= MaximumModels;
	for (uint32_t i = 0; valid && i < count; ++i)
	{
		uint64_t key = 0;
		Parts parts;
		valid = Read(data, offset, key);
		for (auto& model : parts)
		{
			uint32_t vertices = 0;
			if (!valid || !Read(data, offset, vertices) || vertices > 30000 || vertices % 3 ||
			    offset + vertices * sizeof(ModelVertex) > data.size())
			{
				valid = false;
				break;
			}
			model.resize(vertices);
			if (vertices)
				std::memcpy(model.data(), data.data() + offset, vertices * sizeof(ModelVertex));
			offset += vertices * sizeof(ModelVertex);
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
				if (vertex.material < -13 || vertex.material > 12 ||
				    vertex.material != std::floor(vertex.material))
					valid = false;
			}
		}
		valid = valid && !parts[0].empty() && m_Models.emplace(key, std::move(parts)).second;
	}
	if (!valid || offset != data.size())
		m_Models.clear();
}

const VillageModels::Parts* VillageModels::Find(uint64_t key)
{
	Load();
	auto found = m_Models.find(key);
	return found == m_Models.end() ? nullptr : &found->second;
}

const VillageModels::Parts& VillageModels::Store(uint64_t key, Parts parts)
{
	Load();
	m_Dirty = true;
	return m_Models.insert_or_assign(key, std::move(parts)).first->second;
}

void VillageModels::Save()
{
	if (!m_Dirty || m_Models.size() > MaximumModels)
		return;
	std::vector<unsigned char> data;
	Append(data, static_cast<uint32_t>(m_Models.size()));
	for (const auto& entry : m_Models)
	{
		Append(data, entry.first);
		for (const auto& model : entry.second)
		{
			Append(data, static_cast<uint32_t>(model.size()));
			const auto* bytes = reinterpret_cast<const unsigned char*>(model.data());
			if (!model.empty())
				data.insert(data.end(), bytes, bytes + model.size() * sizeof(ModelVertex));
		}
		if (data.size() > MaximumBytes)
			return;
	}
	if (SaveAssetCache(L"village-models-v1.mesh", 1, data))
		m_Dirty = false;
}
