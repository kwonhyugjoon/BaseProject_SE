#pragma once
#include "LevelModels.h"
#include <array>
#include <cstdint>
#include <string>

class FirstLevel
{
  public:
	void Begin(uint32_t seed, bool resetProgress = false);
	void Update(float seconds, const bool* keys);
	bool Key(unsigned char key);
	void Draw(Renderer& renderer, int width, int height, bool postprocess, bool paused);

  private:
	static constexpr int Columns = 48;
	static constexpr int Rows = 40;
	static constexpr float TileSize = 2.f;
	enum class Tile : unsigned char
	{
		Ground,
		Tree,
		Rock,
		Water
	};

	struct Enemy
	{
		Point position, home;
		int hp = 24;
		float cooldown = 1, windup = 0, respawn = 0, flash = 0;
	};

	struct Drop
	{
		Point position;
		int coins, shards;
	};

	std::array<Tile, Columns * Rows> m_Tiles;
	std::array<int, Columns * Rows> m_Distance;
	std::vector<Enemy> m_Enemies;
	std::vector<Drop> m_Drops;
	LevelModels m_Models;
	Point m_Player = {7, 7}, m_Camera = {7, 7}, m_Aim = {1, 0};
	int m_Level = 1, m_Experience = 0, m_Hp = 100, m_Coins = 0, m_Shards = 0;
	int m_Facing = 1, m_Kills = 0, m_FlowTarget = -1;
	uint32_t m_Seed = 1, m_Random = 1;
	float m_Time = 0, m_AttackCooldown = 0, m_Swing = 0, m_HitCooldown = 0, m_MessageTime = 0;
	bool m_Initialized = false, m_Walking = false, m_Map = false, m_Stats = false;
	std::wstring m_Message;
	int AttackPower() const;
	int MaxHp() const;
	int Defense() const;
	static int ExperienceAt(int level);
	uint32_t Random();
	void Generate();
	bool Connected() const;
	int Cell(Point position) const;
	Point Center(int cell) const;
	bool Walkable(Point position) const;
	bool ClearLine(Point from, Point to) const;
	void Move(Point& position, Point delta);
	void RebuildFlow();
	void Step(float dt, const bool* keys);
	void Attack();
	void Pickup();
	void GainExperience(int amount);
	void Message(const std::wstring& text);
	Point Project(Point point, int width, int height) const;
};
