#pragma once
#include "Renderer.h"
#include <array>

enum class LevelModel
{
	Tree,
	Rock,
	Slime,
	Loot,
	Camp,
	Count
};

class LevelModels
{
  public:
	void Load();
	const std::vector<ModelVertex>& Get(LevelModel model) const;

  private:
	std::array<std::vector<ModelVertex>, static_cast<size_t>(LevelModel::Count)> m_Models;
};
