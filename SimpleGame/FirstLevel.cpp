#include "stdafx.h"
#include "FirstLevel.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <iostream>

namespace
{
	float Distance(Point a, Point b)
	{
		return std::hypot(a.x - b.x, a.y - b.y);
	}

	const Point Camp = {7, 7};
	const Color Cream = {.94f, .90f, .76f, 1};
	const Color Gold = {.98f, .73f, .36f, 1};
}

int FirstLevel::AttackPower() const
{
	return 12 + (m_Level - 1) * 4;
}

int FirstLevel::MaxHp() const
{
	return 100 + (m_Level - 1) * 15;
}

int FirstLevel::Defense() const
{
	return m_Level - 1;
}

int FirstLevel::ExperienceAt(int level)
{
	return 15 * level * (level - 1);
}

uint32_t FirstLevel::Random()
{
	m_Random ^= m_Random << 13;
	m_Random ^= m_Random >> 17;
	m_Random ^= m_Random << 5;
	return m_Random;
}

int FirstLevel::Cell(Point p) const
{
	int x = static_cast<int>(std::floor(p.x / TileSize));
	int y = static_cast<int>(std::floor(p.y / TileSize));
	return x >= 0 && y >= 0 && x < Columns && y < Rows ? y * Columns + x : -1;
}

Point FirstLevel::Center(int cell) const
{
	return {(cell % Columns + .5f) * TileSize, (cell / Columns + .5f) * TileSize};
}

bool FirstLevel::Connected() const
{
	std::array<bool, Columns * Rows> seen = {};
	std::queue<int> queue;
	int start = Cell(Camp);
	queue.push(start);
	seen[start] = true;
	int count = 0;
	while (!queue.empty())
	{
		int cell = queue.front();
		queue.pop();
		++count;
		for (int next : {cell - 1, cell + 1, cell - Columns, cell + Columns})
		{
			if (next < 0 || next >= Columns * Rows || seen[next] || m_Tiles[next] != Tile::Ground)
				continue;
			if (std::abs(next % Columns - cell % Columns) +
			        std::abs(next / Columns - cell / Columns) !=
			    1)
				continue;
			seen[next] = true;
			queue.push(next);
		}
	}
	return count == std::count(m_Tiles.begin(), m_Tiles.end(), Tile::Ground);
}

void FirstLevel::Generate()
{
	m_Tiles.fill(Tile::Ground);
	for (int y = 0; y < Rows; ++y)
		for (int x = 0; x < Columns; ++x)
			if (!x || !y || x == Columns - 1 || y == Rows - 1)
				m_Tiles[y * Columns + x] = Tile::Tree;
	// Reject every obstacle that would isolate any traversable cell.
	for (int i = 0; i < 650; ++i)
	{
		int x = 1 + Random() % (Columns - 2), y = 1 + Random() % (Rows - 2);
		int cell = y * Columns + x;
		if (Distance(Center(cell), Camp) < 9 || m_Tiles[cell] != Tile::Ground)
			continue;
		m_Tiles[cell] = static_cast<Tile>(1 + Random() % 3);
		if (!Connected())
			m_Tiles[cell] = Tile::Ground;
	}
	m_Enemies.clear();
	std::vector<int> spawnCells;
	for (int i = 0; i < Columns * Rows; ++i)
		if (m_Tiles[i] == Tile::Ground && Distance(Center(i), Camp) > 12)
			spawnCells.push_back(i);
	// Deterministic shuffle; each spawn is a distinct, reachable cell center.
	for (size_t i = spawnCells.size(); i > 1; --i)
		std::swap(spawnCells[i - 1], spawnCells[Random() % i]);
	for (size_t i = 0; i < std::min(size_t(24), spawnCells.size()); ++i)
	{
		Point p = Center(spawnCells[i]);
		m_Enemies.push_back({p, p});
	}
	m_FlowTarget = -1;
}

void FirstLevel::Begin(uint32_t seed, bool resetProgress)
{
	m_Seed = seed ? seed : 1;
	m_Random = m_Seed;
	if (!m_Initialized || resetProgress)
	{
		m_Level = 1;
		m_Experience = 0;
		m_Hp = 100;
		m_Coins = 0;
		m_Shards = 0;
		m_Kills = 0;
	}
	m_Models.Load();
	m_Initialized = true;
	m_Player = Camp;
	m_Camera = Camp;
	m_Aim = {.70710678f, .70710678f};
	m_Facing = 1;
	m_Drops.clear();
	m_Time = 0;
	m_AttackCooldown = 0;
	m_Swing = 0;
	m_HitCooldown = 0;
	m_Map = false;
	m_Stats = false;
	m_Walking = false;
	Generate();
	std::cout << "First level seed: " << m_Seed << "\n";
	Message(L"슬라임을 사냥하고 전리품을 주워 3레벨에 도달하세요.");
}

bool FirstLevel::Walkable(Point p) const
{
	constexpr float radius = .24f;
	for (float x : {p.x - radius, p.x + radius})
		for (float y : {p.y - radius, p.y + radius})
		{
			int cell = Cell({x, y});
			if (cell < 0 || m_Tiles[cell] != Tile::Ground)
				return false;
		}
	return true;
}

bool FirstLevel::ClearLine(Point from, Point to) const
{
	int steps = std::max(1, static_cast<int>(std::ceil(Distance(from, to) / .2f)));
	for (int i = 1; i <= steps; ++i)
	{
		float t = static_cast<float>(i) / steps;
		if (!Walkable({from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t}))
			return false;
	}
	return true;
}

void FirstLevel::Move(Point& p, Point delta)
{
	if (Walkable({p.x + delta.x, p.y}))
		p.x += delta.x;
	if (Walkable({p.x, p.y + delta.y}))
		p.y += delta.y;
}

void FirstLevel::RebuildFlow()
{
	m_FlowTarget = Cell(m_Player);
	m_Distance.fill(-1);
	std::queue<int> queue;
	queue.push(m_FlowTarget);
	m_Distance[m_FlowTarget] = 0;
	while (!queue.empty())
	{
		int cell = queue.front();
		queue.pop();
		for (int next : {cell - 1, cell + 1, cell - Columns, cell + Columns})
		{
			if (next < 0 || next >= Columns * Rows || m_Distance[next] >= 0 ||
			    m_Tiles[next] != Tile::Ground)
				continue;
			if (std::abs(next % Columns - cell % Columns) +
			        std::abs(next / Columns - cell / Columns) !=
			    1)
				continue;
			m_Distance[next] = m_Distance[cell] + 1;
			queue.push(next);
		}
	}
}

void FirstLevel::Message(const std::wstring& text)
{
	m_Message = text;
	m_MessageTime = 4;
}

void FirstLevel::GainExperience(int amount)
{
	m_Experience = std::min(1000000, m_Experience + amount);
	int previous = m_Level;
	while (m_Level < 10 && m_Experience >= ExperienceAt(m_Level + 1))
		++m_Level;
	if (previous != m_Level)
		Message(L"레벨 상승! Lv." + std::to_wstring(m_Level) + L" · 공격력 " +
		        std::to_wstring(AttackPower()) + L" · 최대 체력 " + std::to_wstring(MaxHp()));
	// Raising maximum HP does not grant a free heal.
}

void FirstLevel::Attack()
{
	m_AttackCooldown = .45f;
	m_Swing = .18f;
	for (auto& enemy : m_Enemies)
	{
		float distance = Distance(enemy.position, m_Player);
		float facing =
		    (enemy.position.x - m_Player.x) * m_Aim.x + (enemy.position.y - m_Player.y) * m_Aim.y;
		if (enemy.hp <= 0 || distance > 2.6f || facing < -.15f * distance ||
		    !ClearLine(m_Player, enemy.position))
			continue;
		enemy.hp = std::max(0, enemy.hp - AttackPower());
		enemy.flash = .15f;
		if (!enemy.hp)
		{
			enemy.respawn = 12;
			enemy.windup = 0;
			m_Drops.push_back({enemy.position, 3, 1});
			++m_Kills;
			GainExperience(15);
		}
	}
}

void FirstLevel::Pickup()
{
	int picked = 0;
	for (auto it = m_Drops.begin(); it != m_Drops.end();)
	{
		if (Distance(it->position, m_Player) < 2.5f && ClearLine(m_Player, it->position))
		{
			m_Coins += it->coins;
			m_Shards += it->shards;
			++picked;
			it = m_Drops.erase(it);
		}
		else
			++it;
	}
	if (picked)
		Message(L"전리품 획득 · 동전 " + std::to_wstring(m_Coins) + L" · 점액 결정 " +
		        std::to_wstring(m_Shards));
}

bool FirstLevel::Key(unsigned char key)
{
	if (m_Hp <= 0)
	{
		if (key == 'r')
			Begin(m_Seed + 1, true);
		return false;
	}
	if (key == 'i')
		m_Stats = !m_Stats;
	if (key == 'm')
		m_Map = !m_Map;
	if (m_Map || m_Stats)
		return false;
	if (key == 'e')
	{
		Pickup();
		if (Distance(m_Player, Camp) < 2.5f)
			return true;
	}
	return false;
}

void FirstLevel::Step(float dt, const bool* keys)
{
	m_AttackCooldown = std::max(0.f, m_AttackCooldown - dt);
	m_Swing = std::max(0.f, m_Swing - dt);
	m_HitCooldown = std::max(0.f, m_HitCooldown - dt);
	float sx = (keys['d'] ? 1.f : 0) - (keys['a'] ? 1.f : 0);
	float sy = (keys['s'] ? 1.f : 0) - (keys['w'] ? 1.f : 0);
	float length = std::hypot(sx, sy);
	if (length > 0)
	{
		float vx = sx / length * 84 * dt, vy = sy / length * 84 * dt;
		Point delta = {(vx / 23 + vy / 11.5f) * .5f, (vy / 11.5f - vx / 23) * .5f};
		float distance = std::hypot(delta.x, delta.y);
		m_Aim = {delta.x / distance, delta.y / distance};
		m_Facing = std::abs(sx) > std::abs(sy) ? (sx > 0 ? 0 : 2) : (sy > 0 ? 1 : 3);
		Point previous = m_Player;
		Move(m_Player, delta);
		m_Walking = m_Walking || Distance(previous, m_Player) > .0001f;
	}
	if (keys[' '] && m_AttackCooldown <= 0)
		Attack();
	if (Cell(m_Player) != m_FlowTarget)
		RebuildFlow();
	for (auto& enemy : m_Enemies)
	{
		enemy.flash = std::max(0.f, enemy.flash - dt);
		if (enemy.hp <= 0)
		{
			enemy.respawn -= dt;
			if (enemy.respawn <= 0 && Distance(m_Player, enemy.home) > 8)
			{
				enemy.position = enemy.home;
				enemy.hp = 24;
				enemy.cooldown = 1;
			}
			continue;
		}
		enemy.cooldown = std::max(0.f, enemy.cooldown - dt);
		float distance = Distance(enemy.position, m_Player);
		bool safe = Distance(m_Player, Camp) < 9;
		if (enemy.windup > 0)
		{
			enemy.windup -= dt;
			if (enemy.windup <= 0)
			{
				enemy.cooldown = 1.2f;
				if (!safe && distance < 1.6f && m_HitCooldown <= 0 &&
				    ClearLine(enemy.position, m_Player))
				{
					m_Hp = std::max(0, m_Hp - std::max(1, 5 - Defense()));
					m_HitCooldown = .6f;
				}
			}
			continue;
		}
		if (safe || distance > 14)
			continue;
		if (distance < 1.5f && ClearLine(enemy.position, m_Player))
		{
			if (enemy.cooldown <= 0)
				enemy.windup = .5f;
			continue;
		}
		int cell = Cell(enemy.position), next = cell;
		for (int candidate : {cell - 1, cell + 1, cell - Columns, cell + Columns})
		{
			if (candidate < 0 || candidate >= Columns * Rows || m_Distance[candidate] < 0)
				continue;
			if (std::abs(candidate % Columns - cell % Columns) +
			        std::abs(candidate / Columns - cell / Columns) !=
			    1)
				continue;
			if (m_Distance[candidate] < m_Distance[next])
				next = candidate;
		}
		Point target = next == m_FlowTarget ? m_Player : Center(next);
		// Reach the current cell center before turning around a blocked corner.
		if (!ClearLine(enemy.position, target))
			target = Center(cell);
		float toTarget = Distance(target, enemy.position);
		if (toTarget > .001f)
		{
			float step = std::min(1.65f * dt, toTarget) / toTarget;
			Move(enemy.position,
			     {(target.x - enemy.position.x) * step, (target.y - enemy.position.y) * step});
		}
	}
	float follow = -std::expm1(-7 * dt);
	m_Camera.x += (m_Player.x - m_Camera.x) * follow;
	m_Camera.y += (m_Player.y - m_Camera.y) * follow;
}

void FirstLevel::Update(float seconds, const bool* keys)
{
	m_Walking = false;
	if (m_Hp <= 0 || m_Map || m_Stats)
		return;
	float remaining = std::min(seconds, .25f);
	m_MessageTime = std::max(0.f, m_MessageTime - remaining);
	while (remaining > 0 && m_Hp > 0)
	{
		float dt = std::min(remaining, 1.f / 120);
		remaining -= dt;
		m_Time += dt;
		Step(dt, keys);
	}
}

Point FirstLevel::Project(Point p, int width, int height) const
{
	return {width * .5f + (p.x - p.y - m_Camera.x + m_Camera.y) * 23,
	        height * .53f + (p.x + p.y - m_Camera.x - m_Camera.y) * 11.5f};
}

void FirstLevel::Draw(Renderer& r, int width, int height, bool postprocess, bool paused)
{
	auto project = [&](Point p)
	{
		return Project(p, width, height);
	};
	auto visible = [&](Point p)
	{
		Point s = project(p);
		return s.x > -100 && s.x < width + 100 && s.y > -50 && s.y < height + 140;
	};
	r.Begin(width, height, m_Time, project({0, 0}));

	struct Object
	{
		Point p;
		LevelModel model;
		int enemy;
		bool player;
	};

	std::vector<Object> objects;
	for (int i = 0; i < Columns * Rows; ++i)
	{
		Point p = Center(i);
		if (!visible(p))
			continue;
		Tile tile = m_Tiles[i];
		std::initializer_list<Point> quad = {project({p.x - 1, p.y - 1}),
		                                     project({p.x + 1, p.y - 1}),
		                                     project({p.x + 1, p.y + 1}),
		                                     project({p.x - 1, p.y + 1})};
		if (tile == Tile::Water)
			r.Polygon(
			    quad, {.19f, .42f, .39f, 1}, Material::Pond, {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}});
		else
		{
			bool camp = Distance(p, Camp) < 9;
			r.Polygon(quad,
			          camp ? Color{.48f, .40f, .27f, 1} : Color{.25f, .34f, .18f, 1},
			          camp ? Material::Dirt : Material::Grass);
			if (tile == Tile::Tree || tile == Tile::Rock)
				objects.push_back(
				    {p, tile == Tile::Tree ? LevelModel::Tree : LevelModel::Rock, -1, false});
		}
	}
	objects.push_back({{Camp.x + 1.5f, Camp.y - 1.5f}, LevelModel::Camp, -1, false});
	for (const auto& drop : m_Drops)
		if (visible(drop.position))
			objects.push_back({drop.position, LevelModel::Loot, -1, false});
	for (size_t i = 0; i < m_Enemies.size(); ++i)
		if (m_Enemies[i].hp > 0 && visible(m_Enemies[i].position))
			objects.push_back(
			    {m_Enemies[i].position, LevelModel::Slime, static_cast<int>(i), false});
	objects.push_back({m_Player, LevelModel::Slime, -1, true});
	for (const auto& object : objects)
	{
		Point s = project(object.p);
		r.SoftEllipse(s.x + 8,
		              s.y + 3,
		              object.model == LevelModel::Tree ? 38.f : 23.f,
		              10,
		              {.025f, .035f, .03f, .32f});
		if (object.enemy >= 0 && m_Enemies[object.enemy].windup > 0)
			r.Ellipse(s.x, s.y, 38, 19, {1, .19f, .10f, .35f});
	}
	std::sort(objects.begin(),
	          objects.end(),
	          [](const Object& a, const Object& b)
	          {
		          return a.p.x + a.p.y < b.p.x + b.p.y;
	          });
	for (const auto& object : objects)
	{
		Point s = project(object.p);
		if (object.player)
		{
			Color coat = m_HitCooldown > 0 ? Color{1, .30f, .25f, 1} : Color{.35f, .49f, .67f, 1};
			r.Character(
			    s.x, s.y, m_Facing, m_Walking ? static_cast<int>(m_Time * 10) % 8 : 8, coat);
			if (m_Swing > 0)
			{
				float angle = std::atan2((m_Aim.x + m_Aim.y) * 11.5f, (m_Aim.x - m_Aim.y) * 23);
				float swing = (1 - m_Swing / .18f) * 1.8f - .9f;
				Point tip = {s.x + std::cos(angle + swing) * 46,
				             s.y - 19 + std::sin(angle + swing) * 30};
				r.Polygon({{s.x - 3, s.y - 17}, tip, {s.x + 3, s.y - 23}},
				          {1.6f, 1.35f, .75f, .9f});
			}
		}
		else
		{
			Color tint = {1, 1, 1, 1};
			if (object.enemy >= 0 && m_Enemies[object.enemy].flash > 0)
				tint = {1.8f, .7f, .6f, 1};
			if (object.model == LevelModel::Tree)
			{
				Point hero = project(m_Player);
				if (s.y > hero.y && s.y - hero.y < 110 && std::abs(s.x - hero.x) < 55)
					tint.a = .1f;
			}
			r.Model(s, m_Models.Get(object.model), tint);
			if (object.enemy >= 0)
			{
				r.Rect(s.x - 18, s.y - 43, 36, 4, {.12f, .08f, .06f, 1});
				r.Rect(s.x - 18,
				       s.y - 43,
				       36.f * m_Enemies[object.enemy].hp / 24,
				       4,
				       {.75f, .26f, .15f, 1});
			}
		}
	}
	r.FinishWorld(postprocess);
	r.Rect(15, 15, 520, 142, {.025f, .055f, .055f, .94f});
	r.Text(30, 24, L"첫 레벨 · 갈대여울 외곽 숲", Gold);
	r.Text(30,
	       51,
	       L"Lv." + std::to_wstring(m_Level) + L"   체력 " + std::to_wstring(m_Hp) + L" / " +
	           std::to_wstring(MaxHp()) + L"   공격 " + std::to_wstring(AttackPower()) +
	           L"   방어 " + std::to_wstring(Defense()),
	       Cream);
	int start = ExperienceAt(m_Level), goal = ExperienceAt(m_Level + 1);
	float progress =
	    m_Level == 10 ? 1.f : static_cast<float>(m_Experience - start) / (goal - start);
	r.Rect(30, 82, 475, 8, {.12f, .19f, .15f, 1});
	r.Rect(30, 82, 475 * progress, 8, {.49f, .72f, .34f, 1});
	r.Text(30,
	       98,
	       L"누적 경험치 " + std::to_wstring(m_Experience) +
	           (m_Level == 10 ? L" · 최고 레벨" : L" / 다음 레벨 " + std::to_wstring(goal)),
	       Cream);
	r.Text(30,
	       126,
	       m_Level >= 3 && m_Shards > 0 ? L"목표 달성! 야영지에서 E로 귀환 · 계속 사냥 가능"
	                                    : L"목표: 3레벨 도달 + 점액 결정 1개 획득",
	       Gold);
	r.Rect(15,
	       static_cast<float>(height - 88),
	       static_cast<float>(width - 30),
	       72,
	       {.025f, .055f, .055f, .94f});
	r.Text(30,
	       static_cast<float>(height - 78),
	       L"WASD 이동 · Space 공격(유지 가능) · E 전리품 줍기/야영지 귀환",
	       Cream);
	r.Text(30,
	       static_cast<float>(height - 49),
	       L"I 가방/스탯 · M 지도 · P 후처리 비교 · Esc 일시정지",
	       Cream);
	if (m_MessageTime > 0)
		r.Text(30, 177, m_Message, Gold);
	if (m_Map)
	{
		float left = width * .5f - Columns * 5.f, top = height * .5f - Rows * 5.f;
		r.Rect(
		    left - 16, top - 40, Columns * 10.f + 32, Rows * 10.f + 64, {.02f, .045f, .04f, .98f});
		for (int i = 0; i < Columns * Rows; ++i)
		{
			Color c = m_Tiles[i] == Tile::Ground  ? Color{.36f, .43f, .27f, 1}
			          : m_Tiles[i] == Tile::Water ? Color{.13f, .35f, .45f, 1}
			                                      : Color{.12f, .20f, .11f, 1};
			r.Rect(left + (i % Columns) * 10.f, top + (i / Columns) * 10.f, 9, 9, c);
		}
		auto mark = [&](Point p, Color c)
		{
			r.Ellipse(left + p.x / 2 * 10, top + p.y / 2 * 10, 4, 4, c);
		};
		for (const auto& enemy : m_Enemies)
			if (enemy.hp > 0)
				mark(enemy.position, {1, .25f, .15f, 1});
		mark(Camp, Gold);
		mark(m_Player, {1, 1, 1, 1});
		r.Text(left, top - 30, L"M 닫기 · 흰색 나 / 금색 야영지 / 붉은색 적", Cream);
	}
	if (m_Stats)
	{
		float x = width * .5f - 240, y = height * .5f - 150;
		r.Rect(x, y, 480, 300, {.025f, .055f, .055f, .98f});
		r.Text(x + 22, y + 20, L"가방과 성장 기록 · I 닫기", Gold);
		r.Text(x + 22,
		       y + 62,
		       L"동전 " + std::to_wstring(m_Coins) + L"     점액 결정 " + std::to_wstring(m_Shards),
		       Cream);
		r.Text(x + 22,
		       y + 102,
		       L"처치 " + std::to_wstring(m_Kills) + L"     누적 경험치 " +
		           std::to_wstring(m_Experience),
		       Cream);
		r.Text(x + 22, y + 145, L"레벨마다 공격 +4 / 최대 체력 +15 / 방어 +1", Cream);
		r.Text(x + 22, y + 182, L"레벨 상승 시 현재 체력은 자동 회복되지 않습니다.", Cream);
		r.Text(x + 22, y + 223, L"전리품과 성장 기록은 이번 실행 동안 유지됩니다.", Cream);
	}
	if (paused || m_Hp <= 0)
	{
		r.Rect(
		    0, 0, static_cast<float>(width), static_cast<float>(height), {.02f, .035f, .03f, .85f});
		r.Text(width * .5f - 220,
		       height * .5f - 30,
		       m_Hp <= 0 ? L"원정 실패 · 체력이 모두 소진되었습니다." : L"일시정지 · Esc로 계속",
		       Gold);
		r.Text(width * .5f - 220,
		       height * .5f + 12,
		       m_Hp <= 0 ? L"R: 경험치·전리품을 초기화하고 새 원정 시작" : L"Q: 게임 종료",
		       Cream);
	}
	r.Flush();
}
