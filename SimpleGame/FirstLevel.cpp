#include "stdafx.h"
#include "FirstLevel.h"
#include "MonitorState.h"
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

FirstLevel::FirstLevel()
    : m_PlayerActor(m_Scene.World().AddChild("Player", Camp)),
      m_StaticActors(m_Scene.World().AddChild("Terrain")),
      m_EnemyActors(m_Scene.World().AddChild("Enemies")),
      m_DropActors(m_Scene.World().AddChild("Drops")), m_Player(m_PlayerActor.Position())
{
	m_Scene.UI().AddChild("HUD").SetDraw(
	    [this](Actor&, Renderer& renderer)
	    {
		    DrawHud(renderer, m_Width, m_Height, m_Paused);
	    });
}

const FirstLevel::EnemyStats& FirstLevel::Stats(EnemyKind kind)
{
	static const EnemyStats stats[] = {
	    {24, 5, 15, 3, 1.65f, 1.5f, .5f, 1.2f, {.3f, .6f, .3f, 1}},
	    {36, 7, 22, 5, 2.2f, 1.7f, .65f, 1.4f, {.62f, .25f, .18f, 1}},
	    {28, 6, 20, 5, 1.9f, 8.f, .9f, 2.f, {.35f, .48f, .22f, 1}},
	    {32, 6, 26, 7, 1.5f, 7.f, 1.15f, 2.4f, {.56f, .3f, .70f, 1}}};
	return stats[static_cast<int>(kind)];
}

void FirstLevel::CaptureMonitorState(MonitorSnapshot& snapshot) const
{
	snapshot.huntingLoaded = m_Initialized;
	const char* kinds[] = {"slime", "bandit", "archer", "priest"};
	const char* names[] = {"슬라임", "도적", "궁수", "타락한 사제"};
	for (size_t i = 0; i < snapshot.mobs.size(); ++i)
	{
		snapshot.mobs[i] = {};
		snapshot.mobs[i].kind = kinds[i];
		snapshot.mobs[i].name = names[i];
	}
	for (const auto& enemy : m_Enemies)
	{
		auto& count = snapshot.mobs[static_cast<size_t>(enemy.kind)];
		++count.total;
		if (enemy.hp > 0)
			++count.alive;
	}
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

bool FirstLevel::NeighborPathsConnected(int cell) const
{
	// Generate removes an interior cell from a connected map with blocked borders.
	// Ring bits run clockwise from north; even bits are the four ground neighbors.
	const Tile* tiles = m_Tiles.data();
	const int ring[] = {
	    -Columns, -Columns + 1, 1, Columns + 1, Columns, Columns - 1, -1, -Columns - 1};
	unsigned ground = 0;
	for (int i = 0; i < 8; ++i)
		if (tiles[cell + ring[i]] == Tile::Ground)
			ground |= 1u << i;

	unsigned targets = ground & 0x55u;
	unsigned reached = targets & (~targets + 1u);
	for (;;)
	{
		unsigned adjacent = (reached << 1) | (reached >> 7) | (reached >> 1) | (reached << 7);
		unsigned expanded = (reached | adjacent) & ground;
		if (expanded == reached)
			break;
		reached = expanded;
	}
	if ((reached & targets) == targets)
		return true;

	// A removed cell can only separate its ground neighbors; stop once they meet.
	int goals[4], goalCount = 0, start = -1, fewestNeighbors = 5;
	const int offsets[] = {-Columns, 1, Columns, -1};
	for (int offset : offsets)
	{
		int neighbor = cell + offset;
		if (tiles[neighbor] != Tile::Ground)
			continue;
		goals[goalCount++] = neighbor;
		int degree = 0;
		for (int next : offsets)
			degree += tiles[neighbor + next] == Tile::Ground;
		if (degree < fewestNeighbors)
		{
			fewestNeighbors = degree;
			start = neighbor;
		}
	}

	bool seen[Columns * Rows] = {};
	int pending[Columns * Rows];
	int read = 0, write = 0, found = 0;
	pending[write++] = start;
	seen[start] = true;
	while (read < write)
	{
		int current = pending[read++];
		for (int i = 0; i < goalCount; ++i)
			found += current == goals[i];
		if (found == goalCount)
			return true;
		for (int offset : offsets)
		{
			int next = current + offset;
			if (next < 0 || next >= Columns * Rows || seen[next] || tiles[next] != Tile::Ground ||
			    std::abs(next % Columns - current % Columns) +
			            std::abs(next / Columns - current / Columns) !=
			        1)
				continue;
			seen[next] = true;
			pending[write++] = next;
		}
	}
	return false;
}

void FirstLevel::Generate()
{
	m_Tiles.fill(Tile::Ground);
	for (int y = 0; y < Rows; ++y)
		for (int x = 0; x < Columns; ++x)
			if (!x || !y || x == Columns - 1 || y == Rows - 1)
				m_Tiles[y * Columns + x] = Tile::Tree;
	// Reject every obstacle that would isolate any traversable cell.
	for (int i = 0; i < 2600; ++i)
	{
		int x = 1 + Random() % (Columns - 2), y = 1 + Random() % (Rows - 2);
		int cell = y * Columns + x;
		if (SafeExit(Center(cell)) || m_Tiles[cell] != Tile::Ground)
			continue;
		m_Tiles[cell] = static_cast<Tile>(1 + Random() % 3);
		if (!NeighborPathsConnected(cell))
			m_Tiles[cell] = Tile::Ground;
	}
	for (auto& tile : m_Tiles)
	{
		if ((WorldTravel::IsDungeon(m_Region) && tile == Tile::Tree) ||
		    (m_Region == Region::StoneRidge && tile == Tile::Water))
			tile = Tile::Rock;
		else if (m_Region == Region::MistShore && tile == Tile::Rock)
			tile = Tile::Water;
	}
	m_Enemies.clear();
	m_EnemyActors.ClearChildren();
	std::vector<int> spawnCells;
	for (int i = 0; i < Columns * Rows; ++i)
		if (m_Tiles[i] == Tile::Ground && !SafeExit(Center(i), 12))
			spawnCells.push_back(i);
	// Deterministic shuffle; each spawn is a distinct, reachable cell center.
	for (size_t i = spawnCells.size(); i > 1; --i)
		std::swap(spawnCells[i - 1], spawnCells[Random() % i]);
	// Three enemies in each 12x10-tile sector: 192 enemies across the expanded map.
	std::array<int, 64> counts = {};
	for (int cell : spawnCells)
	{
		int sector = (cell / Columns / 10) * 8 + cell % Columns / 12;
		if (counts[sector] >= 3)
			continue;
		++counts[sector];
		Point p = Center(cell);
		EnemyKind kind = static_cast<EnemyKind>(m_Enemies.size() % 4);
		// The first encounters near camp remain approachable.
		if (Distance(p, Camp) < 20)
			kind = EnemyKind::Slime;
		const char* names[] = {"Slime/", "Bandit/", "Archer/", "Priest/"};
		Actor& actor = m_EnemyActors.AddChild(
		    names[static_cast<int>(kind)] + std::to_string(m_Enemies.size()), p);
		m_Enemies.emplace_back(actor, p, kind);
	}
	m_FlowTarget = -1;
}

void FirstLevel::Begin(uint32_t seed, bool resetProgress, Region region, Region from)
{
	m_Region = region;
	m_RequestedExit = Exit::None;
	m_ReturnedFromEdge = false;
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
	for (const auto& exit : WorldTravel::Exits(region))
		if (exit.destination == from)
			m_Player = exit.position;
	m_Camera = m_Player;
	m_ReturnTrail.lastSafe = m_Player;
	m_Aim = {.70710678f, .70710678f};
	m_Facing = 1;
	m_Drops.clear();
	m_DropActors.ClearChildren();
	m_Time = 0;
	m_AttackCooldown = 0;
	m_Swing = 0;
	m_HitCooldown = 0;
	m_Map = false;
	m_Stats = false;
	m_Walking = false;
	Generate();
	BuildScene();
	std::cout << "First level seed: " << m_Seed << "\n";
	Message(std::wstring(WorldTravel::Name(region)) + L" · M으로 출구와 목적지를 확인하세요.");
}

bool FirstLevel::SafeExit(Point position, float margin) const
{
	for (const auto& exit : WorldTravel::Exits(m_Region))
		if (Distance(position, exit.position) <= margin)
			return true;
	return !WorldTravel::IsDungeon(m_Region) &&
	       Distance(position, WorldTravel::SurveyStone) <= margin;
}

int FirstLevel::NearbyExit() const
{
	for (size_t i = 0; i < m_Exits.size(); ++i)
		if (m_Exits[i] && Distance(m_Player, m_Exits[i]->WorldPosition()) < 2.5f &&
		    ClearLine(m_Player, m_Exits[i]->WorldPosition()))
			return static_cast<int>(i);
	return -1;
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
		if (!enemy.actor.IsEnabled() || enemy.hp <= 0 || distance > 2.6f ||
		    facing < -.15f * distance || !ClearLine(m_Player, enemy.position))
			continue;
		enemy.hp = std::max(0, enemy.hp - AttackPower());
		enemy.flash = .15f;
		if (!enemy.hp)
		{
			enemy.respawn = 12;
			enemy.windup = 0;
			enemy.actor.SetVisible(false);
			if (m_Drops.size() >= 256)
			{
				Actor* oldest = m_Drops.front().actor;
				m_Drops.erase(m_Drops.begin());
				m_DropActors.RemoveChild(*oldest);
			}
			Actor& drop = m_DropActors.AddChild("Loot/" + std::to_string(m_Kills));
			drop.SetWorldPosition(enemy.position);
			ModelActor(drop, LevelModel::Loot);
			m_Drops.push_back({&drop, Stats(enemy.kind).coins, 1});
			++m_Kills;
			GainExperience(Stats(enemy.kind).experience);
		}
	}
}

void FirstLevel::Pickup()
{
	int picked = 0;
	for (auto it = m_Drops.begin(); it != m_Drops.end();)
	{
		if (Distance(it->actor->Position(), m_Player) < 2.5f &&
		    ClearLine(m_Player, it->actor->Position()))
		{
			m_Coins += it->coins;
			m_Shards += it->shards;
			++picked;
			Actor* actor = it->actor;
			it = m_Drops.erase(it);
			m_DropActors.RemoveChild(*actor);
		}
		else
			++it;
	}
	if (picked)
		Message(L"전리품 획득 · 동전 " + std::to_wstring(m_Coins) + L" · 탐험 결정 " +
		        std::to_wstring(m_Shards));
}

bool FirstLevel::Key(unsigned char key)
{
	if (m_Hp <= 0)
	{
		if (key == 'r')
			Begin(m_Seed + 1, true, m_Region);
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
		int exit = NearbyExit();
		if (exit >= 0)
		{
			m_RequestedExit = WorldTravel::Exits(m_Region)[exit].destination;
			return true;
		}
		if (m_SurveyStone && Distance(m_Player, m_SurveyStone->WorldPosition()) < 2.5f)
			Message(L"이정표에는 산맥·고원·해안의 표식이 있다. 다음 여행의 단서를 기록했다.");
	}
	return false;
}

void FirstLevel::Step(float dt, const bool* keys)
{
	m_Input = keys;
	m_Scene.Update(dt);

	m_Camera = m_Player;
}

void FirstLevel::UpdatePlayer(float dt)
{
	m_AttackCooldown = std::max(0.f, m_AttackCooldown - dt);
	m_Swing = std::max(0.f, m_Swing - dt);
	m_HitCooldown = std::max(0.f, m_HitCooldown - dt);
	float sx = (m_Input['d'] ? 1.f : 0) - (m_Input['a'] ? 1.f : 0);
	float sy = (m_Input['s'] ? 1.f : 0) - (m_Input['w'] ? 1.f : 0);
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
	if (m_ReturnTrail.ReturnIfLost(m_Player, Columns * TileSize, Rows * TileSize))
	{
		m_ReturnedFromEdge = true;
		m_Walking = false;
		m_FlowTarget = -1;
		m_HitCooldown = std::max(m_HitCooldown, .6f);
		Message(WorldTravel::IsDungeon(m_Region)
		            ? L"어두운 통로에서 방향을 잃고 헤매다 익숙한 갈림길로 돌아왔습니다."
		            : L"숲에서 길을 잃고 한참 헤매다 익숙한 길로 돌아왔습니다.");
		return;
	}
	if (m_Input[' '] && m_AttackCooldown <= 0)
		Attack();
	if (Cell(m_Player) != m_FlowTarget)
		RebuildFlow();
}

void FirstLevel::UpdateEnemy(Enemy& enemy, float dt)
{
	const EnemyStats& stats = Stats(enemy.kind);
	enemy.moving = false;
	enemy.glow = std::max(0.f, enemy.glow - dt);
	enemy.flash = std::max(0.f, enemy.flash - dt);
	if (enemy.hp <= 0)
	{
		enemy.respawn -= dt;
		if (enemy.respawn <= 0 && Distance(m_Player, enemy.home) > 8)
		{
			enemy.position = enemy.home;
			enemy.hp = Stats(enemy.kind).hp;
			enemy.supportCooldown = 3;
			enemy.glow = 0;
			enemy.actor.SetVisible(true);
			enemy.cooldown = 1;
		}
		return;
	}
	enemy.cooldown = std::max(0.f, enemy.cooldown - dt);
	float distance = Distance(enemy.position, m_Player);
	bool safe = SafeExit(m_Player);
	if (enemy.windup > 0)
	{
		enemy.windup -= dt;
		if (enemy.windup <= 0)
		{
			enemy.cooldown = stats.cooldown;
			bool ranged = enemy.kind == EnemyKind::Archer || enemy.kind == EnemyKind::Priest;
			bool hit = ranged ? Distance(m_Player, enemy.attackTarget) < 1.1f
			                  : distance < stats.range + .1f;
			if (!safe && hit && distance < stats.range + 1 && m_HitCooldown <= 0 &&
			    ClearLine(enemy.position, m_Player))
			{
				m_Hp = std::max(0, m_Hp - std::max(1, stats.damage - Defense()));
				m_HitCooldown = .6f;
			}
		}
		return;
	}
	if (safe || distance > 24)
		return;
	// Support work is throttled; distant enemies never scan their allies.
	enemy.supportCooldown -= dt;
	if (enemy.kind == EnemyKind::Priest && enemy.supportCooldown <= 0)
	{
		enemy.supportCooldown = 4;
		Enemy* target = nullptr;
		for (auto& ally : m_Enemies)
			if (ally.hp > 0 && ally.hp < Stats(ally.kind).hp && ally.actor.IsEnabled() &&
			    Distance(ally.position, enemy.position) < 7 &&
			    ClearLine(enemy.position, ally.position) && (!target || ally.hp < target->hp))
				target = &ally;
		if (target)
		{
			target->hp = std::min(Stats(target->kind).hp, target->hp + 8);
			target->glow = .6f;
			enemy.glow = .6f;
			enemy.cooldown = std::max(enemy.cooldown, 1.f);
			return;
		}
	}
	if (distance > 14)
	{
		Point target = {enemy.home.x + std::sin(m_Time * .3f + enemy.home.x) * 1.5f,
		                enemy.home.y + std::cos(m_Time * .3f + enemy.home.y) * 1.5f};
		float length = Distance(target, enemy.position);
		if (length > .1f && ClearLine(enemy.position, target))
		{
			float step = std::min(stats.speed * .35f * dt, length) / length;
			Point previous = enemy.position;
			Move(enemy.position,
			     {(target.x - enemy.position.x) * step, (target.y - enemy.position.y) * step});
			enemy.moving = Distance(previous, enemy.position) > .00001f;
		}
		return;
	}
	// Ranged enemies retreat when approached, without crossing blocked cells.
	if ((enemy.kind == EnemyKind::Archer || enemy.kind == EnemyKind::Priest) && distance < 3 &&
	    distance > .01f)
	{
		Point previous = enemy.position;
		float step = stats.speed * dt / distance;
		Move(enemy.position,
		     {(enemy.position.x - m_Player.x) * step, (enemy.position.y - m_Player.y) * step});
		enemy.moving = Distance(previous, enemy.position) > .00001f;
	}
	if (distance < stats.range && ClearLine(enemy.position, m_Player))
	{
		if (enemy.cooldown <= 0)
		{
			enemy.windup = stats.windup;
			enemy.attackTarget = m_Player;
		}
		return;
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
		float step = std::min(stats.speed * dt, toTarget) / toTarget;
		Point previous = enemy.position;
		Move(enemy.position,
		     {(target.x - enemy.position.x) * step, (target.y - enemy.position.y) * step});
		enemy.moving = Distance(previous, enemy.position) > .00001f;
		Point delta = {enemy.position.x - previous.x, enemy.position.y - previous.y};
		enemy.facing = std::abs(delta.x - delta.y) > std::abs(delta.x + delta.y)
		                   ? (delta.x > delta.y ? 0 : 2)
		                   : (delta.x + delta.y > 0 ? 1 : 3);
	}
}

bool FirstLevel::Update(float seconds, const bool* keys)
{
	m_Walking = false;
	if (m_Hp <= 0 || m_Map || m_Stats)
		return false;
	float remaining = std::min(seconds, .25f);
	m_MessageTime = std::max(0.f, m_MessageTime - remaining);
	while (remaining > 0 && m_Hp > 0)
	{
		float dt = std::min(remaining, 1.f / 120);
		remaining -= dt;
		m_Time += dt;
		Step(dt, keys);
		if (m_ReturnedFromEdge)
		{
			m_ReturnedFromEdge = false;
			return true;
		}
	}
	return false;
}

Point FirstLevel::Project(Point p, int width, int height) const
{
	return {width * .5f + (p.x - p.y - m_Camera.x + m_Camera.y) * 23,
	        height * .53f + (p.x + p.y - m_Camera.x - m_Camera.y) * 11.5f};
}

void FirstLevel::ModelActor(Actor& actor, LevelModel model, int enemy)
{
	actor.SetLayer(ActorLayer::World);
	actor.SetDraw(
	    [this, model, enemy](Actor& node, Renderer& r)
	    {
		    Point s = Project(node.Position(), m_Width, m_Height);
		    if (s.x < -100 || s.x > m_Width + 100 || s.y < -50 || s.y > m_Height + 140)
			    return;
		    Color tint = {1, 1, 1, 1};
		    if (enemy >= 0 && m_Enemies[enemy].flash > 0)
			    tint = {1.8f, .7f, .6f, 1};
		    if (model == LevelModel::Tree)
		    {
			    Point hero = Project(m_Player, m_Width, m_Height);
			    auto smooth = [](float value)
			    {
				    value = std::max(0.f, std::min(1.f, value));
				    return value * value * (3 - 2 * value);
			    };
			    float dy = s.y - hero.y;
			    float overlap = smooth((55 - std::abs(s.x - hero.x)) / 20) * smooth(dy / 20) *
			                    smooth((110 - dy) / 30);
			    tint.a = 1 - .9f * overlap;
		    }
		    if (enemy >= 0 && m_Enemies[enemy].kind != EnemyKind::Slime)
		    {
			    const Enemy& foe = m_Enemies[enemy];
			    Color coat = Stats(foe.kind).color;
			    if (foe.flash > 0)
				    coat = {1, .25f, .2f, 1};
			    r.Character(
			        s.x, s.y, foe.facing, foe.moving ? static_cast<int>(m_Time * 10) % 8 : 8, coat);
			    if (foe.kind == EnemyKind::Priest)
			    {
				    r.Rect(s.x + 13, s.y - 52, 3, 50, {.52f, .36f, .2f, 1});
				    r.Ellipse(s.x + 14, s.y - 54, 5, 5, {.9f, .5f, 1.4f, 1});
			    }
			    else if (foe.kind == EnemyKind::Archer)
				    r.Polygon({{s.x + 12, s.y - 43}, {s.x + 25, s.y - 27}, {s.x + 12, s.y - 10}},
				              {.6f, .4f, .2f, 1});
			    else
				    r.Polygon({{s.x + 12, s.y - 12}, {s.x + 28, s.y - 38}, {s.x + 31, s.y - 32}},
				              {.85f, .85f, .75f, 1});
		    }
		    else
			    r.Model(s, m_Models.Get(model), tint);
		    if (enemy >= 0 && m_Enemies[enemy].glow > 0)
			    r.SoftEllipse(s.x, s.y - 25, 27, 35, {.2f, 1.3f, .55f, .35f});
	    });
	Actor& shadow = actor.AddChild("Shadow");
	shadow.SetLayer(ActorLayer::Shadow);
	shadow.SetDraw(
	    [this, model, enemy](Actor& node, Renderer& r)
	    {
		    Point s = Project(node.Position(), m_Width, m_Height);
		    if (s.x < -100 || s.x > m_Width + 100 || s.y < -50 || s.y > m_Height + 140)
			    return;
		    r.SoftEllipse(s.x + 8,
		                  s.y + 3,
		                  model == LevelModel::Tree ? 38.f : 23.f,
		                  10,
		                  {.025f, .035f, .03f, .32f});
		    if (enemy >= 0 && m_Enemies[enemy].windup > 0)
		    {
			    const Enemy& foe = m_Enemies[enemy];
			    Point warning = foe.kind == EnemyKind::Archer || foe.kind == EnemyKind::Priest
			                        ? Project(foe.attackTarget, m_Width, m_Height)
			                        : s;
			    r.Ellipse(warning.x, warning.y, 38, 19, {1, .19f, .10f, .35f});
		    }
	    });
	if (enemy >= 0)
		actor.AddChild("HealthBar")
		    .SetDraw(
		        [this, enemy](Actor& node, Renderer& r)
		        {
			        Point s = Project(node.Position(), m_Width, m_Height);
			        if (s.x < -100 || s.x > m_Width + 100 || s.y < -50 || s.y > m_Height + 140)
				        return;
			        r.Rect(s.x - 18, s.y - 43, 36, 4, {.12f, .08f, .06f, 1});
			        r.Rect(s.x - 18,
			               s.y - 43,
			               36.f * m_Enemies[enemy].hp / Stats(m_Enemies[enemy].kind).hp,
			               4,
			               {.75f, .26f, .15f, 1});
		        });
}

void FirstLevel::BuildScene()
{
	m_StaticActors.ClearChildren();
	m_PlayerActor.ClearChildren();
	m_Exits.clear();
	m_SurveyStone = nullptr;
	// Aggregate ground tiles in 8x8 groups; props still have independent Actor ownership.
	for (int cy = 0; cy < Rows; cy += 8)
		for (int cx = 0; cx < Columns; cx += 8)
		{
			Actor& chunk = m_StaticActors.AddChild("Ground/" + std::to_string(cy * Columns + cx),
			                                       {cx * TileSize, cy * TileSize});
			chunk.SetLayer(ActorLayer::Ground);
			chunk.SetDraw(
			    [this, cx, cy](Actor& actor, Renderer& r)
			    {
				    Point origin = actor.Position();
				    Point middle = Project({origin.x + 8, origin.y + 8}, m_Width, m_Height);
				    if (middle.x < -450 || middle.x > m_Width + 450 || middle.y < -250 ||
				        middle.y > m_Height + 250)
					    return;
				    for (int y = 0; y < 8 && cy + y < Rows; ++y)
					    for (int x = 0; x < 8 && cx + x < Columns; ++x)
					    {
						    int i = (cy + y) * Columns + cx + x;
						    Point p = {origin.x + (x + .5f) * TileSize,
						               origin.y + (y + .5f) * TileSize};
						    auto project = [this](Point point)
						    {
							    return Project(point, m_Width, m_Height);
						    };
						    Point screen = project(p);
						    if (screen.x < -100 || screen.x > m_Width + 100 || screen.y < -50 ||
						        screen.y > m_Height + 140)
							    continue;
						    std::initializer_list<Point> quad = {project({p.x - 1, p.y - 1}),
						                                         project({p.x + 1, p.y - 1}),
						                                         project({p.x + 1, p.y + 1}),
						                                         project({p.x - 1, p.y + 1})};
						    if (m_Tiles[i] == Tile::Water)
							    r.Polygon(quad,
							              {.19f, .42f, .39f, 1},
							              Material::Pond,
							              {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}});
						    else
						    {
							    bool camp = Distance(p, Camp) < 9;
							    r.Polygon(
							        quad,
							        camp                               ? Color{.48f, .40f, .27f, 1}
							        : WorldTravel::IsDungeon(m_Region) ? Color{.32f, .35f, .33f, 1}
							        : m_Region == Region::StoneRidge   ? Color{.39f, .40f, .29f, 1}
							        : m_Region == Region::MistShore    ? Color{.24f, .36f, .35f, 1}
							                                           : Color{.25f, .34f, .18f, 1},
							        camp                               ? Material::Dirt
							        : WorldTravel::IsDungeon(m_Region) ? Material::Stone
							                                           : Material::Grass);
						    }
					    }
			    });
			for (int y = cy; y < cy + 8 && y < Rows; ++y)
				for (int x = cx; x < cx + 8 && x < Columns; ++x)
				{
					int i = y * Columns + x;
					if (m_Tiles[i] != Tile::Tree && m_Tiles[i] != Tile::Rock)
						continue;
					Point p = Center(i);
					Actor& prop = chunk.AddChild("Prop/" + std::to_string(i),
					                             {p.x - cx * TileSize, p.y - cy * TileSize});
					ModelActor(prop,
					           m_Tiles[i] == Tile::Tree ? LevelModel::Tree : LevelModel::Rock);
				}
		}
	ModelActor(m_StaticActors.AddChild("Campfire", {Camp.x + 1.5f, Camp.y - 1.5f}),
	           LevelModel::Camp);
	for (const auto& exit : WorldTravel::Exits(m_Region))
	{
		Actor& gate =
		    m_StaticActors.AddChild("Exit/" + std::to_string(m_Exits.size()), exit.position);
		m_Exits.push_back(&gate);
		bool daylight = !WorldTravel::IsDungeon(exit.destination);
		gate.SetDraw(
		    [this, daylight](Actor& actor, Renderer& r)
		    {
			    Point screen = Project(actor.WorldPosition(), m_Width, m_Height);
			    if (screen.x > -100 && screen.x < m_Width + 100 && screen.y > -50 &&
			        screen.y < m_Height + 150)
				    WorldTravel::DrawGate(r, screen, daylight);
		    });
	}
	if (!WorldTravel::IsDungeon(m_Region))
	{
		m_SurveyStone = &m_StaticActors.AddChild("SurveyStone", WorldTravel::SurveyStone);
		ModelActor(*m_SurveyStone, LevelModel::Rock);
	}
	for (int cy = -48; cy < Rows * 2 + 48; cy += 16)
		for (int cx = -48; cx < Columns * 2 + 48; cx += 16)
		{
			if (cx >= 0 && cy >= 0 && cx < Columns * 2 && cy < Rows * 2)
				continue;
			Actor& border =
			    m_StaticActors.AddChild("Boundary/" + std::to_string(cy) + "/" + std::to_string(cx),
			                            {static_cast<float>(cx), static_cast<float>(cy)});
			border.SetLayer(ActorLayer::Ground);
			border.SetDraw(
			    [this](Actor& actor, Renderer& r)
			    {
				    Point p = actor.WorldPosition();
				    r.Polygon({Project(p, m_Width, m_Height),
				               Project({p.x + 16, p.y}, m_Width, m_Height),
				               Project({p.x + 16, p.y + 16}, m_Width, m_Height),
				               Project({p.x, p.y + 16}, m_Width, m_Height)},
				              WorldTravel::IsDungeon(m_Region) ? Color{.24f, .27f, .25f, 1}
				                                               : Color{.20f, .29f, .17f, 1},
				              WorldTravel::IsDungeon(m_Region) ? Material::Stone : Material::Grass);
			    });
			for (int y = 3; y < 16; y += 5)
				for (int x = 3; x < 16; x += 5)
					ModelActor(border.AddChild("Tree/" + std::to_string(y * 16 + x),
					                           {static_cast<float>(x), static_cast<float>(y)}),
					           WorldTravel::IsDungeon(m_Region) ? LevelModel::Rock
					                                            : LevelModel::Tree);
		}
	for (size_t i = 0; i < m_Enemies.size(); ++i)
	{
		ModelActor(m_Enemies[i].actor, LevelModel::Slime, static_cast<int>(i));
		m_Enemies[i].actor.SetUpdate(
		    [this, i](Actor&, float dt)
		    {
			    UpdateEnemy(m_Enemies[i], dt);
		    });
	}
	m_PlayerActor.SetUpdate(
	    [this](Actor&, float dt)
	    {
		    if (m_Input)
			    UpdatePlayer(dt);
	    });
	m_PlayerActor.SetDraw(
	    [this](Actor& actor, Renderer& r)
	    {
		    Point s = Project(actor.Position(), m_Width, m_Height);
		    Color coat = m_HitCooldown > 0 ? Color{1, .30f, .25f, 1} : Color{.35f, .49f, .67f, 1};
		    r.Character(
		        s.x, s.y, m_Facing, m_Walking ? static_cast<int>(m_Time * 10) % 8 : 8, coat);
	    });
	Actor& shadow = m_PlayerActor.AddChild("Shadow");
	shadow.SetLayer(ActorLayer::Shadow);
	shadow.SetDraw(
	    [this](Actor& actor, Renderer& r)
	    {
		    Point s = Project(actor.Position(), m_Width, m_Height);
		    r.SoftEllipse(s.x + 8, s.y + 3, 23, 10, {.025f, .035f, .03f, .32f});
	    });
	m_PlayerActor.AddChild("Weapon").SetDraw(
	    [this](Actor& actor, Renderer& r)
	    {
		    if (m_Swing <= 0)
			    return;
		    Point s = Project(actor.Position(), m_Width, m_Height);
		    float angle = std::atan2((m_Aim.x + m_Aim.y) * 11.5f, (m_Aim.x - m_Aim.y) * 23);
		    float swing = (1 - m_Swing / .18f) * 1.8f - .9f;
		    Point tip = {s.x + std::cos(angle + swing) * 46,
		                 s.y - 19 + std::sin(angle + swing) * 30};
		    r.Polygon({{s.x - 3, s.y - 17}, tip, {s.x + 3, s.y - 23}}, {1.6f, 1.35f, .75f, .9f});
	    });
}

void FirstLevel::Draw(Renderer& r, int width, int height, bool postprocess, bool paused)
{
	m_Width = width;
	m_Height = height;
	m_Paused = paused;
	for (const auto& chunk : m_StaticActors.Children())
	{
		if (chunk->Layer() != ActorLayer::Ground)
			continue;
		Point origin = chunk->WorldPosition();
		Point center = Project({origin.x + 8, origin.y + 8}, width, height);
		chunk->SetVisible(center.x >= -450 && center.x <= width + 450 && center.y >= -250 &&
		                  center.y <= height + 250);
	}
	r.Begin(width, height, m_Time, Project({0, 0}, width, height));
	m_Scene.DrawWorld(r);
	r.FinishWorld(postprocess);
	m_Scene.DrawUI(r);
}

void FirstLevel::DrawHud(Renderer& r, int width, int height, bool paused)
{
	r.Rect(15, 15, 520, 142, {.025f, .055f, .055f, .94f});
	r.Text(30,
	       24,
	       std::wstring(WorldTravel::Name(m_Region)) +
	           (WorldTravel::IsDungeon(m_Region) ? L" · 고대 교역 통로" : L" · 탐험 지역"),
	       Gold);
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
	       m_Level >= 3 && m_Shards > 0
	           ? L"성장 목표 달성 · 출입구에서 E로 다음 지역 이동"
	           : L"성장 목표: 3레벨 + 탐험 결정 1개 · 출구는 언제든 이용 가능",
	       Gold);
	r.Rect(15,
	       static_cast<float>(height - 88),
	       static_cast<float>(width - 30),
	       72,
	       {.025f, .055f, .055f, .94f});
	r.Text(30,
	       static_cast<float>(height - 78),
	       L"WASD 이동 · Space 공격(유지 가능) · E 전리품/출입구 상호작용",
	       Cream);
	r.Text(30,
	       static_cast<float>(height - 49),
	       L"I 가방/스탯 · M 지도 · P 후처리 비교 · Esc 일시정지",
	       Cream);
	if (m_MessageTime > 0)
		r.Text(30, 177, m_Message, Gold);
	int nearbyExit = NearbyExit();
	if (nearbyExit >= 0 && !m_Map && !m_Stats && !paused && m_Hp > 0)
		r.Text(30,
		       height - 120.f,
		       std::wstring(L"[E] ") +
		           WorldTravel::Name(WorldTravel::Exits(m_Region)[nearbyExit].destination) +
		           L" 이동",
		       Gold);
	if (m_Map)
	{
		float scale = std::min((width - 100.f) / Columns, (height - 140.f) / Rows);
		float left = (width - Columns * scale) * .5f, top = (height - Rows * scale) * .5f;
		r.Rect(left - 16,
		       top - 40,
		       Columns * scale + 32,
		       Rows * scale + 64,
		       {.02f, .045f, .04f, .98f});
		for (int i = 0; i < Columns * Rows; ++i)
		{
			Color c = m_Tiles[i] == Tile::Ground  ? Color{.36f, .43f, .27f, 1}
			          : m_Tiles[i] == Tile::Water ? Color{.13f, .35f, .45f, 1}
			                                      : Color{.12f, .20f, .11f, 1};
			r.Rect(left + (i % Columns) * scale, top + (i / Columns) * scale, scale, scale, c);
		}
		auto mark = [&](Point p, Color c)
		{
			r.Ellipse(left + p.x / 2 * scale, top + p.y / 2 * scale, 4, 4, c);
		};
		for (const auto& enemy : m_Enemies)
			if (enemy.hp > 0)
				mark(enemy.position, {1, .25f, .15f, 1});
		for (size_t i = 0; i < m_Exits.size(); ++i)
		{
			Point position = m_Exits[i]->WorldPosition();
			mark(position, i == 0 ? Gold : Color{.3f, 1, .85f, 1});
			float labelX = std::min(left + position.x / 2 * scale, left + (Columns - 24) * scale);
			r.Text(labelX,
			       top + position.y / 2 * scale + 8,
			       WorldTravel::Name(WorldTravel::Exits(m_Region)[i].destination),
			       Cream);
		}
		if (m_SurveyStone)
			mark(m_SurveyStone->WorldPosition(), {.8f, .7f, .4f, 1});
		mark(m_Player, {1, 1, 1, 1});
		r.Text(left, top - 30, L"M 닫기 · 흰색: 나 / 노랑·청록: 출입구와 목적지 / 빨강: 적", Cream);
	}
	if (m_Stats)
	{
		float x = width * .5f - 240, y = height * .5f - 150;
		r.Rect(x, y, 480, 300, {.025f, .055f, .055f, .98f});
		r.Text(x + 22, y + 20, L"가방과 성장 기록 · I 닫기", Gold);
		r.Text(x + 22,
		       y + 62,
		       L"동전 " + std::to_wstring(m_Coins) + L"     탐험 결정 " + std::to_wstring(m_Shards),
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
