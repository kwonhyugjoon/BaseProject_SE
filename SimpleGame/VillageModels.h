#pragma once
#include "Renderer.h"
#include <array>
#include <cstdint>
#include <map>

class VillageModels
{
  public:
	using Parts = std::array<std::vector<ModelVertex>, 6>;
	static uint64_t Key(std::initializer_list<float> parameters);
	const Parts* Find(uint64_t key);
	const Parts& Store(uint64_t key, Parts parts);
	void Save();

  private:
	void Load();
	std::map<uint64_t, Parts> m_Models;
	bool m_Loaded = false, m_Dirty = false;
};
