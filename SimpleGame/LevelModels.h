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
	Wolf,
	Beetle,
	Guardian,
	Dwarf,
	Archive,
	DwarfHall,
	TrailLoot,
	OreLoot,
	PartsLoot,
	ScaleLizard,
	WindSprite,
	Emberling,
	Dragon,
	ScaleLoot,
	WindLoot,
	EmberLoot,
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
