#include "stdafx.h"
#include "FirstLevel.h"
#include "QuestJournal.h"
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

	const Point Camp = WorldTravel::NearGate;
	const Color Cream = {.94f, .90f, .76f, 1};
	const Color Gold = {.98f, .73f, .36f, 1};
}

FirstLevel::FirstLevel(QuestJournal& journal)
    : m_Journal(journal), m_PlayerActor(m_Scene.World().AddChild("Player", Camp)),
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
	    {32, 6, 26, 7, 1.5f, 7.f, 1.15f, 2.4f, {.56f, .3f, .70f, 1}},
	    {28, 7, 20, 3, 2.6f, 4.f, .7f, 2.1f, {.44f, .47f, .42f, 1}},
	    {38, 6, 24, 4, 1.35f, 1.5f, .85f, 1.8f, {.57f, .46f, .25f, 1}},
	    {56, 10, 36, 6, 1.1f, 3.6f, 1.3f, 2.8f, {.46f, .52f, .50f, 1}},
	    {46, 8, 30, 6, 1.8f, 5.f, 1.f, 2.6f, {.30f, .48f, .51f, 1}},
	    {30, 6, 28, 5, 2.3f, 7.f, 1.1f, 2.5f, {.58f, .85f, .89f, 1}},
	    {34, 8, 32, 6, 1.4f, 6.f, 1.4f, 4.5f, {.94f, .38f, .13f, 1}}};
	return stats[static_cast<int>(kind)];
}

void FirstLevel::CaptureMonitorState(MonitorSnapshot& snapshot) const
{
	snapshot.huntingLoaded = m_Initialized;
	const char* kinds[] = {"slime",
	                       "bandit",
	                       "archer",
	                       "priest",
	                       "wolf",
	                       "beetle",
	                       "guardian",
	                       "scale_lizard",
	                       "wind_sprite",
	                       "emberling"};
	const char* names[] = {"슬라임",
	                       "도적",
	                       "궁수",
	                       "타락한 사제",
	                       "숲 늑대",
	                       "광맥 갑충",
	                       "고장 난 석재 수호체",
	                       "능선 비늘도마뱀",
	                       "바람 정령",
	                       "열맥 불씨"};
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

	const Region areas[] = {Region::StoneRidge,
	                        Region::DwarfCity,
	                        Region::DwarfCity,
	                        Region::Highlands,
	                        Region::DragonRoost};
	const Point positions[] = {
	    {55, 41}, WorldTravel::SurveyStone, {57, 43}, {55, 41}, WorldTravel::SurveyStone};
	const char* ids[] = {
	    "Dwarf/Torvik", "Dwarf/Bryn", "Dwarf/Dorna", "Highlands/Sera", "Dragon/Arken"};
	const char* labels[] = {
	    "파수꾼 토르빅", "기록관 브린", "대장장이 도르나", "길잡이 세라", "고룡 아르켄"};
	for (int i = 0; i < 5; ++i)
	{
		bool active =
		    m_Initialized && m_Region == areas[i] && snapshot.scene == WorldTravel::Key(areas[i]);
		Actor* node = active ? m_StaticActors.Find(ids[i]) : nullptr;
		MonitorNpc npc;
		npc.id = ids[i];
		npc.name = labels[i];
		npc.villageId = i < 3 ? "deepstone" : "highlands";
		npc.routine = i == 2 ? "대장간 안내" : "기록과 길 안내";
		npc.state = !active ? "inactive_area"
		            : IsStoryOpen() &&
		                    Distance(m_Player, node ? node->WorldPosition() : positions[i]) < 2.5f
		                ? "talking"
		            : snapshot.paused ? "paused"
		                              : "standing";
		Point position = node ? node->WorldPosition() : positions[i];
		npc.x = position.x;
		npc.y = position.y;
		npc.visible = node && node->IsVisible();
		npc.enabled = node && node->IsEnabled();
		if (node && !node->IsEnabled())
			npc.state = "disabled";
		snapshot.npcs.push_back(npc);
	}
	MonitorVillage city;
	city.id = "deepstone";
	city.name = "깊은돌 도시와 전초지";
	city.population = 3;
	city.mood = m_Journal.DwarfChoice() == 1   ? "교역 재개를 준비함"
	            : m_Journal.DwarfChoice() == 2 ? "안전 점검에 집중함"
	                                           : "신중한 환대";
	city.disposition = m_Journal.DwarfChoice() == 1   ? "감시 아래 교류"
	                   : m_Journal.DwarfChoice() == 2 ? "봉인 유지와 기록 보존"
	                                                  : "기록을 통해 판단";
	city.rumorsImplemented = true;
	city.rumors = {m_Journal.DwarfChoice() == 1   ? "분지 지름길의 감시 등불이 다시 켜졌다."
	               : m_Journal.DwarfChoice() == 2 ? "지름길 봉인은 유지되고 산길 교역은 계속된다."
	                                              : "폐광 수호체가 광부의 귀환 신호를 기다린다."};
	snapshot.villages.push_back(city);
	MonitorVillage highlands;
	highlands.id = "highlands";
	highlands.name = "바람등 고원과 쉼터";
	highlands.population = 2;
	highlands.mood = m_Journal.DragonChoice() == 3 ? "서로 거리를 지킴"
	                 : m_Journal.DragonChoice()    ? "경계의 기억을 나눔"
	                                               : "낯선 발걸음을 관찰함";
	highlands.disposition = m_Journal.DragonChoice() == 1   ? "서식지 보존 협약"
	                        : m_Journal.DragonChoice() == 2 ? "관찰자 인정"
	                        : m_Journal.DragonChoice() == 3 ? "바람길 통행 거부"
	                                                        : "아직 판단하지 않음";
	highlands.rumorsImplemented = true;
	highlands.rumors = {
	    m_Journal.DragonChoice() == 3 ? "바람길은 닫혀 있지만 고원으로 돌아갈 수 있다."
	    : m_Journal.DragonChoice()    ? "고룡이 돌등성이 바람길을 열었다."
	                                  : "비늘은 전쟁이 아니라 탈피의 흔적이라는 이야기가 돈다."};
	snapshot.villages.push_back(highlands);
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
	for (int i = 0; m_Region != Region::DwarfCity && m_Region != Region::DragonRoost &&
	                i < 2600 * Columns * Rows / (96 * 80);
	     ++i)
	{
		int x = 1 + Random() % (Columns - 2), y = 1 + Random() % (Rows - 2);
		int cell = y * Columns + x;
		if (SafeExit(Center(cell)) || OnTravelRoad(Center(cell)) || m_Tiles[cell] != Tile::Ground)
			continue;
		m_Tiles[cell] = static_cast<Tile>(1 + Random() % 3);
		if (!NeighborPathsConnected(cell))
			m_Tiles[cell] = Tile::Ground;
	}
	for (auto& tile : m_Tiles)
	{
		if (((m_Region == Region::StoneRidge || m_Region == Region::Highlands) &&
		     tile == Tile::Water) ||
		    (m_Region == Region::DwarfMine && (tile == Tile::Water || tile == Tile::Tree)))
			tile = Tile::Rock;
		else if (m_Region == Region::MistShore && tile == Tile::Rock)
			tile = Tile::Water;
	}
	m_Enemies.clear();
	m_EnemyActors.ClearChildren();
	if (m_Region == Region::DwarfCity || m_Region == Region::DragonRoost)
	{
		m_FlowTarget = -1;
		return;
	}
	std::vector<int> spawnCells;
	for (int i = 0; i < Columns * Rows; ++i)
		if (m_Tiles[i] == Tile::Ground && !SafeExit(Center(i), 12))
			spawnCells.push_back(i);
	// Deterministic shuffle; each spawn is a distinct, reachable cell center.
	for (size_t i = spawnCells.size(); i > 1; --i)
		std::swap(spawnCells[i - 1], spawnCells[Random() % i]);
	// Keep three enemies per 12x10-tile sector as the region becomes more compact.
	std::array<int, (Columns / 12) * (Rows / 10)> counts = {};
	for (int cell : spawnCells)
	{
		int sector = (cell / Columns / 10) * (Columns / 12) + cell % Columns / 12;
		if (counts[sector] >= 3)
			continue;
		++counts[sector];
		Point p = Center(cell);
		static const EnemyKind road[] = {EnemyKind::Bandit, EnemyKind::Archer, EnemyKind::Wolf};
		static const EnemyKind ridge[] = {
		    EnemyKind::Wolf, EnemyKind::Beetle, EnemyKind::Bandit, EnemyKind::Archer};
		static const EnemyKind mine[] = {EnemyKind::Beetle, EnemyKind::Guardian};
		static const EnemyKind highland[] = {
		    EnemyKind::ScaleLizard, EnemyKind::WindSprite, EnemyKind::Emberling};
		EnemyKind kind;
		if (WorldTravel::IsPassage(m_Region))
			kind = road[m_Enemies.size() % 3];
		else if (m_Region == Region::StoneRidge)
			kind = ridge[m_Enemies.size() % 4];
		else if (m_Region == Region::DwarfMine)
			kind = mine[m_Enemies.size() % 2];
		else if (m_Region == Region::Highlands)
			kind = highland[m_Enemies.size() % 3];
		else
		{
			kind = static_cast<EnemyKind>(m_Enemies.size() % 4);
			if (Distance(p, Camp) < 20)
				kind = EnemyKind::Slime;
		}
		const char* names[] = {"Slime/",
		                       "Bandit/",
		                       "Archer/",
		                       "Priest/",
		                       "Wolf/",
		                       "Beetle/",
		                       "Guardian/",
		                       "ScaleLizard/",
		                       "WindSprite/",
		                       "Emberling/"};
		Actor& actor = m_EnemyActors.AddChild(
		    names[static_cast<int>(kind)] + std::to_string(m_Enemies.size()), p);
		m_Enemies.emplace_back(actor, p, kind);
	}
	m_FlowTarget = -1;
}

void FirstLevel::ResetProgress()
{
	m_Level = 1;
	m_Experience = 0;
	m_Hp = 100;
	m_Coins = m_Shards = m_Kills = 0;
	m_Materials.fill(0);
}

void FirstLevel::ClearInput()
{
	m_AttackQueued = false;
	m_Input = nullptr;
}

bool FirstLevel::IsSimulationPaused() const
{
	return m_Hp <= 0 || m_Map || m_Stats || IsStoryOpen() || m_Journal.IsOpen();
}

void FirstLevel::Reset()
{
	ResetProgress();
	ClearInput();
	m_Initialized = false;
	m_Enemies.clear();
	m_Drops.clear();
	m_Exits.clear();
	m_EnemyActors.ClearChildren();
	m_DropActors.ClearChildren();
	m_StaticActors.ClearChildren();
	m_PlayerActor.ClearChildren();
	m_SurveyStone = m_StoryContact = m_Smith = m_DragonTrace = nullptr;
	m_Dialogue.clear();
	m_DialoguePage = 0;
	m_StoryAction = StoryAction::None;
	m_ChoosingDwarf = m_ChoosingDragon = m_Map = m_Stats = m_Walking = false;
	m_TrialWaves = 0;
	m_TrialWindup = 0;
	m_Message.clear();
	m_RequestedExit = Exit::None;
	m_Time = m_Swing = m_AttackCooldown = m_HitCooldown = m_MessageTime = 0;
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
		ResetProgress();
	ClearInput();
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
	m_Dialogue.clear();
	m_DialoguePage = 0;
	m_ChoosingDwarf = false;
	m_ChoosingDragon = false;
	m_TrialWaves = 0;
	m_StoryAction = StoryAction::None;
	Generate();
	BuildScene();
	m_Journal.Visit(region);
	std::cout << "First level seed: " << m_Seed << "\n";
	Message(std::wstring(WorldTravel::Name(region)) + L" · M으로 출구와 목적지를 확인하세요.");
}

bool FirstLevel::SafeExit(Point position, float margin) const
{
	if (m_Region == Region::DwarfCity || m_Region == Region::DragonRoost)
		return true;
	if ((m_Region == Region::StoneRidge || m_Region == Region::Highlands) &&
	    Distance(position, {55, 41}) <= margin)
		return true;
	for (const auto& exit : WorldTravel::Exits(m_Region))
		if (Distance(position, exit.position) <= margin)
			return true;
	return !WorldTravel::IsPassage(m_Region) &&
	       Distance(position, WorldTravel::SurveyStone) <= margin;
}

bool FirstLevel::OnTravelRoad(Point position) const
{
	for (const auto& exit : WorldTravel::Exits(m_Region))
	{
		Point center = WorldTravel::SurveyStone;
		float x = center.x - exit.position.x, y = center.y - exit.position.y;
		float lengthSquared = x * x + y * y;
		float t = lengthSquared > 0
		              ? ((position.x - exit.position.x) * x + (position.y - exit.position.y) * y) /
		                    lengthSquared
		              : 0;
		t = std::max(0.f, std::min(1.f, t));
		if (Distance(position, {exit.position.x + x * t, exit.position.y + y * t}) < 2.8f)
			return true;
	}
	return false;
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
		int armor = (enemy.kind == EnemyKind::Beetle || enemy.kind == EnemyKind::ScaleLizard) &&
		                    enemy.windup <= 0
		                ? 3
		                : 0;
		enemy.hp = std::max(0, enemy.hp - std::max(1, AttackPower() - armor));
		enemy.flash = .15f;
		if (!enemy.hp)
		{
			enemy.respawn = 12;
			enemy.windup = 0;
			enemy.charge = 0;
			enemy.heat = 0;
			enemy.actor.SetVisible(false);
			if (m_Drops.size() >= 256)
			{
				Actor* oldest = m_Drops.front().actor;
				m_Drops.erase(m_Drops.begin());
				m_DropActors.RemoveChild(*oldest);
			}
			Actor& drop = m_DropActors.AddChild("Loot/" + std::to_string(m_Kills));
			drop.SetWorldPosition(enemy.position);
			int material = enemy.kind == EnemyKind::Beetle        ? 1
			               : enemy.kind == EnemyKind::Guardian    ? 2
			               : enemy.kind == EnemyKind::ScaleLizard ? 3
			               : enemy.kind == EnemyKind::WindSprite  ? 4
			               : enemy.kind == EnemyKind::Emberling   ? 5
			                                                      : 0;
			LevelModel lootModels[] = {LevelModel::TrailLoot,
			                           LevelModel::OreLoot,
			                           LevelModel::PartsLoot,
			                           LevelModel::ScaleLoot,
			                           LevelModel::WindLoot,
			                           LevelModel::EmberLoot};
			ModelActor(drop, lootModels[material]);
			m_Drops.push_back({&drop, Stats(enemy.kind).coins, 1, material});
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
		if (Distance(it->actor->WorldPosition(), m_Player) < 2.5f &&
		    ClearLine(m_Player, it->actor->WorldPosition()))
		{
			m_Coins += it->coins;
			m_Shards += it->shards;
			if (it->material >= 0)
				++m_Materials[it->material];
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
	if (m_Journal.IsOpen())
		return false;
	if (m_Hp <= 0)
	{
		if (key == 'r')
			Begin(m_Seed + 1, true, m_Region);
		return false;
	}
	if (m_ChoosingDwarf)
	{
		if ((key == '1' || key == '2') && m_Journal.ChooseDwarf(key - '0'))
		{
			m_ChoosingDwarf = false;
			GainExperience(75);
			Message(m_Journal.DwarfChoice() == 1
			            ? L"교역 재개 · 분지와 폐광 사이 지름길이 열렸습니다."
			            : L"봉인 유지 · 기존 돌등성이 산길로 계속 왕래할 수 있습니다.");
		}
		return false;
	}
	if (m_ChoosingDragon)
	{
		if (key >= '1' && key <= '3' && m_Journal.ChooseDragon(key - '0'))
		{
			m_ChoosingDragon = false;
			if (key == '2')
				StartDragonTrial();
			else
				InteractDragon();
		}
		return false;
	}
	if (!m_Dialogue.empty())
	{
		if ((key == 'e' || key == ' ') && ++m_DialoguePage == m_Dialogue.size())
			FinishStory();
		return false;
	}
	if (key == 'i')
	{
		m_Stats = !m_Stats;
		ClearInput();
	}
	if (key == 'm')
	{
		m_Map = !m_Map;
		ClearInput();
	}
	if (m_Map || m_Stats)
		return false;
	if (key == ' ')
		m_AttackQueued = true;
	if (key == 'e')
	{
		Pickup();
		int exit = NearbyExit();
		if (exit >= 0)
		{
			Region destination = WorldTravel::Exits(m_Region)[exit].destination;
			if (!m_Journal.CanTravel(m_Region, destination))
			{
				Message(
				    destination == Region::DragonRoost && m_Region == Region::Highlands
				        ? L"쉼터의 표식이 흐릿합니다. 길잡이와 대화하고 중앙의 경계문을 먼저 조사하세요."
				    : destination == Region::DragonRoost || m_Region == Region::DragonRoost
				        ? L"고룡의 바람길은 아직 닫혀 있습니다. 협상이나 시험 뒤 기억을 들으면 열립니다."
				    : destination == Region::DwarfCity
				        ? L"도시의 파수꾼: 폐광 작업 기록을 확인한 여행자만 맞이합니다. 기록을 먼저 조사하세요."
				        : L"봉인된 분지 지름길입니다. 도시 기록관에게 교역 재개 여부를 결정하세요.");
				return false;
			}
			m_RequestedExit = destination;
			return true;
		}
		if (m_DragonTrace && Distance(m_Player, m_DragonTrace->WorldPosition()) < 2.5f &&
		    ClearLine(m_Player, m_DragonTrace->WorldPosition()))
		{
			InteractDragon();
			return false;
		}
		if ((m_StoryContact && Distance(m_Player, m_StoryContact->WorldPosition()) < 2.5f &&
		     ClearLine(m_Player, m_StoryContact->WorldPosition())) ||
		    (m_Smith && Distance(m_Player, m_Smith->WorldPosition()) < 2.5f))
		{
			if (m_Region == Region::Highlands || m_Region == Region::DragonRoost)
				InteractDragon();
			else
				InteractStory();
			return false;
		}
		if (m_SurveyStone && Distance(m_Player, m_SurveyStone->WorldPosition()) < 2.5f &&
		    ClearLine(m_Player, m_SurveyStone->WorldPosition()))
			Message(
			    m_Journal.Investigate(m_Region)
			        ? L"새 이정표 단서를 여행 일지에 기록했습니다. J로 다음 목적지를 확인하세요."
			        : L"이미 기록한 이정표입니다. J로 단서와 다음 목적지를 확인하세요.");
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
		float speed = m_Input[GameInput::Sprint] ? 168.f : 84.f;
		float vx = sx / length * speed * dt, vy = sy / length * speed * dt;
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
		Message(WorldTravel::IsPassage(m_Region)
		            ? L"표식 없는 샛길에서 방향을 잃고 헤매다 익숙한 갈림길로 돌아왔습니다."
		            : L"숲에서 길을 잃고 한참 헤매다 익숙한 길로 돌아왔습니다.");
		return;
	}
	if ((m_AttackQueued || m_Input[' ']) && m_AttackCooldown <= 0)
	{
		m_AttackQueued = false;
		Attack();
	}
	if (Cell(m_Player) != m_FlowTarget)
		RebuildFlow();
}

void FirstLevel::UpdateEnemy(Enemy& enemy, float dt)
{
	const EnemyStats& stats = Stats(enemy.kind);
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
			enemy.charge = 0;
			enemy.actor.SetVisible(true);
			enemy.cooldown = 1;
		}
		return;
	}
	enemy.cooldown = std::max(0.f, enemy.cooldown - dt);
	float distance = Distance(enemy.position, m_Player);
	bool safe = SafeExit(m_Player);
	if (enemy.heat > 0)
	{
		enemy.heat = std::max(0.f, enemy.heat - dt);
		if (enemy.heat > 0 && !safe && Distance(enemy.attackTarget, m_Player) < 2 &&
		    m_HitCooldown <= 0 && ClearLine(enemy.position, m_Player))
		{
			m_Hp = std::max(0, m_Hp - std::max(1, stats.damage - Defense()));
			m_HitCooldown = .6f;
		}
	}
	if (enemy.charge > 0)
	{
		enemy.charge = std::max(0.f, enemy.charge - dt);
		Point previous = enemy.position;
		float speed = enemy.kind == EnemyKind::ScaleLizard ? 6.f : 8.f;
		Move(enemy.position,
		     {enemy.chargeDirection.x * speed * dt, enemy.chargeDirection.y * speed * dt});
		enemy.moving = enemy.moving || Distance(previous, enemy.position) > .00001f;
		if (!safe && Distance(enemy.position, m_Player) < 1.3f && m_HitCooldown <= 0 &&
		    ClearLine(enemy.position, m_Player))
		{
			m_Hp = std::max(0, m_Hp - std::max(1, stats.damage - Defense()));
			m_HitCooldown = .6f;
			enemy.charge = 0;
		}
		return;
	}
	if (enemy.windup > 0)
	{
		enemy.windup -= dt;
		if (enemy.windup <= 0)
		{
			enemy.cooldown = stats.cooldown;
			if (enemy.kind == EnemyKind::Emberling)
			{
				enemy.heat = 3;
				return;
			}
			bool ranged = enemy.kind == EnemyKind::Archer || enemy.kind == EnemyKind::Priest ||
			              enemy.kind == EnemyKind::WindSprite;
			if (enemy.kind == EnemyKind::Wolf || enemy.kind == EnemyKind::ScaleLizard)
			{
				float length = Distance(enemy.position, enemy.attackTarget);
				if (!safe && length > .01f)
				{
					enemy.chargeDirection = {(enemy.attackTarget.x - enemy.position.x) / length,
					                         (enemy.attackTarget.y - enemy.position.y) / length};
					enemy.charge = enemy.kind == EnemyKind::ScaleLizard
					                   ? std::min(.75f, length / 6)
					                   : std::min(.45f, length / 8);
				}
				return;
			}
			bool hit = enemy.kind == EnemyKind::Guardian
			               ? Distance(m_Player, enemy.attackTarget) < 2.3f
			           : ranged ? Distance(m_Player, enemy.attackTarget) < 1.1f
			                    : distance < stats.range + .1f;
			if (!safe && hit && distance < stats.range + 1 && m_HitCooldown <= 0 &&
			    ClearLine(enemy.position, m_Player))
			{
				m_Hp = std::max(0, m_Hp - std::max(1, stats.damage - Defense()));
				m_HitCooldown = .6f;
				if (enemy.kind == EnemyKind::WindSprite && distance > .01f)
				{
					float dx = (m_Player.x - enemy.position.x) / distance;
					float dy = (m_Player.y - enemy.position.y) / distance;
					for (int i = 0; i < 10; ++i)
					{
						Point next = {m_Player.x + dx * .15f, m_Player.y + dy * .15f};
						if (SafeExit(next) ||
						    WorldTravel::OuterEdge(next, Columns * TileSize, Rows * TileSize, 4))
							break;
						Move(m_Player, {dx * .15f, dy * .15f});
					}
				}
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
			enemy.moving = enemy.moving || Distance(previous, enemy.position) > .00001f;
		}
		return;
	}
	// Ranged enemies retreat when approached, without crossing blocked cells.
	if ((enemy.kind == EnemyKind::Archer || enemy.kind == EnemyKind::Priest ||
	     enemy.kind == EnemyKind::WindSprite || enemy.kind == EnemyKind::Emberling) &&
	    distance < 3 && distance > .01f)
	{
		Point previous = enemy.position;
		float step = stats.speed * dt / distance;
		Move(enemy.position,
		     {(enemy.position.x - m_Player.x) * step, (enemy.position.y - m_Player.y) * step});
		enemy.moving = enemy.moving || Distance(previous, enemy.position) > .00001f;
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
		bool moved = Distance(previous, enemy.position) > .00001f;
		enemy.moving = enemy.moving || moved;
		Point delta = {enemy.position.x - previous.x, enemy.position.y - previous.y};
		if (moved)
			enemy.facing = std::abs(delta.x - delta.y) > std::abs(delta.x + delta.y)
			                   ? (delta.x > delta.y ? 0 : 2)
			                   : (delta.x + delta.y > 0 ? 1 : 3);
	}
}

bool FirstLevel::Update(float seconds, const bool* keys)
{
	m_Walking = false;
	for (auto& enemy : m_Enemies)
		enemy.moving = false;
	if (IsSimulationPaused())
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

LevelModel FirstLevel::EnemyModel(EnemyKind kind) const
{
	switch (kind)
	{
	case EnemyKind::Wolf:
		return LevelModel::Wolf;
	case EnemyKind::Beetle:
		return LevelModel::Beetle;
	case EnemyKind::Guardian:
		return LevelModel::Guardian;
	case EnemyKind::ScaleLizard:
		return LevelModel::ScaleLizard;
	case EnemyKind::WindSprite:
		return LevelModel::WindSprite;
	case EnemyKind::Emberling:
		return LevelModel::Emberling;
	default:
		return LevelModel::Slime;
	}
}

void FirstLevel::BuildDwarfScene()
{
	if (m_Region == Region::StoneRidge)
	{
		m_StoryContact = &m_StaticActors.AddChild("Dwarf/Torvik", {55, 41});
		ModelActor(*m_StoryContact, LevelModel::Dwarf);
	}
	else if (m_Region == Region::DwarfMine)
	{
		m_StoryContact = &m_StaticActors.AddChild("MineRecord", WorldTravel::SurveyStone);
		ModelActor(*m_StoryContact, LevelModel::Archive);
	}
	else if (m_Region == Region::DwarfCity)
	{
		m_StoryContact = &m_StaticActors.AddChild("Dwarf/Bryn", WorldTravel::SurveyStone);
		m_Smith = &m_StaticActors.AddChild("Dwarf/Dorna", {57, 43});
		ModelActor(*m_StoryContact, LevelModel::Dwarf);
		ModelActor(*m_Smith, LevelModel::Dwarf);
		for (Point p : {Point{37, 31}, Point{63, 47}})
			ModelActor(
			    m_StaticActors.AddChild("CityArch/" + std::to_string(static_cast<int>(p.x)), p),
			    LevelModel::DwarfHall);
		ModelActor(m_StaticActors.AddChild("CityForge", {57, 47}), LevelModel::Camp);
	}
}

Point FirstLevel::StoryTarget() const
{
	if (m_DragonTrace && m_Journal.DragonProgress() == QuestJournal::DragonStage::Trace)
		return m_DragonTrace->WorldPosition();
	if (m_StoryContact && (m_Region != Region::StoneRidge || m_Journal.Investigated(m_Region)))
		return m_StoryContact->WorldPosition();
	return WorldTravel::SurveyStone;
}

void FirstLevel::InteractStory()
{
	using DwarfStage = QuestJournal::DwarfStage;
	m_DialoguePage = 0;
	m_StoryAction = StoryAction::None;
	if (m_Smith && Distance(m_Player, m_Smith->WorldPosition()) < 2.5f)
	{
		m_Dialogue = {
		    L"대장장이 도르나: 갑충의 껍질에는 광맥이 붙어 있고 수호체에는 길을 안내하던 부품이 남아 있어. 둘을 같은 짐승이라 여기지 마.",
		    m_Journal.DwarfChoice() == 1
		        ? L"교역을 다시 시작하면 수리할 부품을 받을 수 있겠지. 대신 경비 교대도 늘려야 해. 우리가 함께 책임질 일이야."
		    : m_Journal.DwarfChoice() == 2
		        ? L"봉인을 남겨 두었으니 안전 점검부터 하겠어. 돌아갈 산길은 열려 있어. 단절을 영원한 적대로 여기진 말아 줘."
		        : L"갑충은 공격을 준비할 때 껍질의 틈이 벌어져. 수호체의 큰 타격은 바닥에 새겨지는 예고를 보고 피할 수 있어."};
	}
	else if (m_Region == Region::StoneRidge)
	{
		if (m_Journal.TutorialStage() != QuestJournal::Stage::Complete ||
		    !m_Journal.Investigated(Region::StoneRidge))
			m_Dialogue = {
			    L"파수꾼 토르빅: 갈대여울의 부탁을 마치고 옆 이정표를 기록해 줘. 표식을 읽는 여행자에게 우리 도시의 이야기를 맡기고 싶어."};
		else if (m_Journal.DwarfProgress() == DwarfStage::Meeting)
		{
			m_Dialogue = {
			    L"파수꾼 토르빅: 지도 기록자라면 부탁이 있네. 깊은돌 폐광길에서 옛 작업 기록을 찾아 도시의 기록관 브린에게 전해 주게.",
			    L"수호체는 원래 귀환하는 광부의 등불을 따라 길을 열던 장치였어. 고장 나 지금은 낯선 발걸음에 반응하지. 무조건 파괴할 필요는 없네.",
			    L"중앙 기록대까지 길은 남아 있네. 기록에는 닫힌 지름길의 이유가 있을 거야. 마을의 종에 답했든 거리를 두었든 우리는 자네의 기록을 듣겠네."};
			m_StoryAction = StoryAction::Start;
		}
		else
			m_Dialogue = {
			    m_Journal.DwarfChoice() == 1
			        ? L"토르빅: 분지와 폐광 지름길에 감시 등불을 켰네. 여행자가 오가면 소문보다 서로의 모습을 더 잘 알게 되겠지."
			    : m_Journal.DwarfChoice() == 2
			        ? L"토르빅: 봉인은 유지하지만 이 산길로는 계속 오갈 수 있네. 다음 여행에도 도시가 한 선택을 기억해 주게."
			        : L"토르빅: 동쪽 출입구가 깊은돌 폐광길로 이어져. 중앙 작업 기록을 읽은 뒤 도시 앞마당의 브린을 찾아가게."};
	}
	else if (m_Region == Region::DwarfMine)
	{
		if (m_Journal.DwarfProgress() == DwarfStage::Record)
		{
			m_Dialogue = {
			    L"폐광 작업 기록: 이 길은 물자와 사람을 위한 길이었다. 경계를 긁어 얻은 광석을 실어 나른 뒤, 낯선 울림이 귀환 신호를 흉내 내기 시작했다.",
			    L"도시는 채굴과 분지 지름길을 멈췄다. 수호체에는 마지막 귀환 명령이 남았다. 돌아가는 이에게 표식을. 갈대여울에서 본 문구와 닮았다.",
			    L"기록의 인장을 옮겨 그렸다. 도시 문에서 이 기록을 제시할 수 있다. 원래의 기록과 봉인 장치는 그대로 두었다."};
			m_StoryAction = StoryAction::Record;
		}
		else
			m_Dialogue = {
			    m_Journal.DwarfProgress() == DwarfStage::Meeting
			        ? L"작업 기록대: 드워프 인장과 귀환 표식이 있다. 먼저 돌등성이의 토르빅에게 이 표식이 무엇인지 물어보자."
			        : L"작업 기록은 이미 일지에 옮겼다. 도시 문은 기록을 확인한 여행자에게 열려 있다. J로 다음 목적지를 확인하자."};
	}
	else
	{
		if (m_Journal.DwarfProgress() == DwarfStage::Report)
		{
			m_Dialogue = {
			    L"기록관 브린: 우리는 낯선 이를 모두 적으로 여겨 길을 닫은 게 아니야. 길을 지킬 책임보다 채굴을 앞세운 우리에게도 잘못이 있었지.",
			    L"감시 아래 옛 교역길을 열면 분지와 바로 오가며 부품과 소식을 나눌 수 있어. 대신 울림을 지켜볼 사람이 더 필요해.",
			    L"봉인을 유지하면 조사할 시간을 얻지만 교역은 먼 산길을 돌아야 하지. 어느 쪽이든 도시는 네 기록을 남길 거야. 우리의 다음 걸음을 함께 정해 줘."};
			m_StoryAction = StoryAction::Council;
		}
		else if (m_Journal.DwarfProgress() == DwarfStage::Choice)
			m_ChoosingDwarf = true;
		else
			m_Dialogue = {
			    m_Journal.DwarfChoice() == 1
			        ? L"브린: 도시는 여행자를 교역 협력자로 맞이한다. 분지 지름길을 열고 감시와 수리를 함께 맡기로 했다. 길을 여는 것은 책임을 나누는 일이겠지."
			    : m_Journal.DwarfChoice() == 2
			        ? L"브린: 도시는 기록 보존과 안전 점검에 집중한다. 지름길은 닫혀 있어도 기존 산길과 우리의 대화는 열려 있다."
			        : L"브린: 먼저 폐광의 작업 기록을 읽고 와 줘. 이 도시의 판단에는 우리가 남긴 기록이 필요해."};
	}
}

void FirstLevel::FinishStory()
{
	m_Dialogue.clear();
	m_DialoguePage = 0;
	if (m_StoryAction == StoryAction::Start)
		m_Journal.StartDwarfStory();
	else if (m_StoryAction == StoryAction::Record)
		m_Journal.ReadDwarfRecord();
	else if (m_StoryAction == StoryAction::Council && m_Journal.HearDwarfCouncil())
		m_ChoosingDwarf = true;
	else if (m_StoryAction == StoryAction::DragonStart)
		m_Journal.StartDragonStory();
	else if (m_StoryAction == StoryAction::DragonTrace)
		m_Journal.ReadDragonTrace();
	else if (m_StoryAction == StoryAction::DragonAudience && m_Journal.HearDragon())
		m_ChoosingDragon = true;
	else if (m_StoryAction == StoryAction::DragonMemory && m_Journal.ReadDragonMemory())
	{
		GainExperience(75);
		Message(m_Journal.DragonChoice() == 3
		            ? L"고룡과 거리 두기 · 고원으로 돌아가는 길은 유지됩니다."
		            : L"고룡의 기억 기록 · 돌등성이로 향하는 바람길이 열렸습니다.");
	}
	m_StoryAction = StoryAction::None;
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
		    Point s = Project(node.WorldPosition(), m_Width, m_Height);
		    float margin = model == LevelModel::Dragon ? 140.f : 100.f;
		    if (s.x < -margin || s.x > m_Width + margin || s.y < -50 || s.y > m_Height + 140)
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
		    if (enemy >= 0 && m_Enemies[enemy].kind >= EnemyKind::Bandit &&
		        m_Enemies[enemy].kind <= EnemyKind::Priest)
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
		    {
			    if (model == LevelModel::WindSprite)
				    s.y -= 5 + std::sin(m_Time * 3 + node.WorldPosition().x) * 4;
			    if (enemy >= 0 && m_Enemies[enemy].moving)
				    s.y -=
				        std::abs(std::sin(m_Time * (model == LevelModel::Guardian ? 4 : 12))) * 2;
			    r.Model(s, m_Models.Get(model), tint);
		    }
		    if (enemy >= 0 && m_Enemies[enemy].glow > 0)
			    r.SoftEllipse(s.x, s.y - 25, 27, 35, {.2f, 1.3f, .55f, .35f});
	    });
	Actor& shadow = actor.AddChild("Shadow");
	shadow.SetLayer(ActorLayer::Shadow);
	shadow.SetDraw(
	    [this, model, enemy](Actor& node, Renderer& r)
	    {
		    Point s = Project(node.WorldPosition(), m_Width, m_Height);
		    bool warning = enemy >= 0 && (m_Enemies[enemy].windup > 0 || m_Enemies[enemy].heat > 0);
		    if (!warning &&
		        (s.x < -100 || s.x > m_Width + 100 || s.y < -50 || s.y > m_Height + 140))
			    return;
		    r.SoftEllipse(s.x + 8,
		                  s.y + 3,
		                  model == LevelModel::Dragon ? 85.f
		                  : model == LevelModel::Tree ? 38.f
		                                              : 23.f,
		                  model == LevelModel::Dragon ? 25.f : 10.f,
		                  {.025f, .035f, .03f, .32f});
		    if (enemy >= 0 && m_Enemies[enemy].windup > 0)
		    {
			    const Enemy& foe = m_Enemies[enemy];
			    Point warning =
			        foe.kind == EnemyKind::Archer || foe.kind == EnemyKind::Priest ||
			                foe.kind == EnemyKind::Wolf || foe.kind == EnemyKind::Guardian ||
			                foe.kind == EnemyKind::ScaleLizard ||
			                foe.kind == EnemyKind::WindSprite || foe.kind == EnemyKind::Emberling
			            ? Project(foe.attackTarget, m_Width, m_Height)
			            : s;
			    float radius = foe.kind == EnemyKind::Guardian || foe.kind == EnemyKind::Emberling
			                       ? 76.f
			                       : 38.f;
			    r.Ellipse(warning.x, warning.y, radius, radius * .5f, {1, .19f, .10f, .35f});
			    if (foe.kind == EnemyKind::ScaleLizard)
				    r.Polygon({{s.x - 4, s.y}, {s.x + 4, s.y}, warning}, {1, .50f, .16f, .35f});
		    }
		    if (enemy >= 0 && m_Enemies[enemy].heat > 0)
		    {
			    Point p = Project(m_Enemies[enemy].attackTarget, m_Width, m_Height);
			    r.Ellipse(p.x, p.y, 65, 32, {1.2f, .31f, .08f, .48f});
			    r.Polygon(
			        {{p.x - 30, p.y}, {p.x + 30, p.y}, {p.x + 30, p.y - 32}, {p.x - 30, p.y - 32}},
			        {1, 1, 1, .75f},
			        Material::Flame,
			        {{-1, 0}, {1, 0}, {1, 1}, {-1, 1}});
		    }
	    });
	if (enemy >= 0)
		actor.AddChild("HealthBar")
		    .SetDraw(
		        [this, enemy](Actor& node, Renderer& r)
		        {
			        Point s = Project(node.WorldPosition(), m_Width, m_Height);
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
	m_StoryContact = m_Smith = nullptr;
	m_DragonTrace = nullptr;
	Color terrainColor = m_Region == Region::Highlands     ? Color{.43f, .48f, .35f, 1}
	                     : m_Region == Region::DragonRoost ? Color{.40f, .37f, .32f, 1}
	                     : m_Region == Region::DwarfMine   ? Color{.34f, .33f, .28f, 1}
	                     : m_Region == Region::DwarfCity   ? Color{.43f, .43f, .36f, 1}
	                     : m_Region == Region::StoneRidge  ? Color{.39f, .40f, .29f, 1}
	                     : m_Region == Region::MistShore   ? Color{.24f, .36f, .35f, 1}
	                                                       : Color{.25f, .34f, .18f, 1};
	// Aggregate ground tiles in 8x8 groups; props still have independent Actor ownership.
	for (int cy = 0; cy < Rows; cy += 8)
		for (int cx = 0; cx < Columns; cx += 8)
		{
			Actor& chunk = m_StaticActors.AddChild("Ground/" + std::to_string(cy * Columns + cx),
			                                       {cx * TileSize, cy * TileSize});
			chunk.SetLayer(ActorLayer::Ground);
			chunk.SetDraw(
			    [this, cx, cy, terrainColor](Actor& actor, Renderer& r)
			    {
				    Point origin = actor.WorldPosition();
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
							    bool camp = Distance(p, Camp) < 9 || OnTravelRoad(p);
							    r.Polygon(quad,
							              camp ? Color{.48f, .40f, .27f, 1} : terrainColor,
							              m_Region == Region::DwarfCity   ? Material::Stone
							              : camp                          ? Material::Dirt
							              : m_Region == Region::DwarfMine ? Material::Stone
							                                              : Material::Grass);
						    }
					    }
			    });
			for (int y = cy; y < cy + 8 && y < Rows; ++y)
				for (int x = cx; x < cx + 8 && x < Columns; ++x)
				{
					int i = y * Columns + x;
					if ((m_Tiles[i] != Tile::Tree && m_Tiles[i] != Tile::Rock) || x == 0 ||
					    y == 0 || x == Columns - 1 || y == Rows - 1)
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
		Region destination = exit.destination;
		gate.SetDraw(
		    [this, destination](Actor& actor, Renderer& r)
		    {
			    Point screen = Project(actor.WorldPosition(), m_Width, m_Height);
			    if (screen.x > -100 && screen.x < m_Width + 100 && screen.y > -50 &&
			        screen.y < m_Height + 150)
			    {
				    WorldTravel::DrawGate(r, screen, true);
				    if (!m_Journal.CanTravel(m_Region, destination))
					    for (int x = -18; x <= 18; x += 9)
						    r.Rect(screen.x + x, screen.y - 53, 3, 53, {.50f, .40f, .22f, 1});
			    }
		    });
	}
	if (m_Region == Region::Basin || m_Region == Region::StoneRidge ||
	    m_Region == Region::MistShore)
	{
		m_SurveyStone = &m_StaticActors.AddChild("SurveyStone", WorldTravel::SurveyStone);
		ModelActor(*m_SurveyStone, LevelModel::Rock);
	}
	for (int cy = -WorldTravel::VisualMargin; cy < Rows * 2 + WorldTravel::VisualMargin; cy += 16)
		for (int cx = -WorldTravel::VisualMargin; cx < Columns * 2 + WorldTravel::VisualMargin;
		     cx += 16)
		{
			if (cx >= 0 && cy >= 0 && cx < Columns * 2 && cy < Rows * 2)
				continue;
			Actor& border =
			    m_StaticActors.AddChild("Boundary/" + std::to_string(cy) + "/" + std::to_string(cx),
			                            {static_cast<float>(cx), static_cast<float>(cy)});
			border.SetLayer(ActorLayer::Ground);
			border.SetDraw(
			    [this, terrainColor](Actor& actor, Renderer& r)
			    {
				    Point p = actor.WorldPosition();
				    r.Polygon({Project(p, m_Width, m_Height),
				               Project({p.x + 16, p.y}, m_Width, m_Height),
				               Project({p.x + 16, p.y + 16}, m_Width, m_Height),
				               Project({p.x, p.y + 16}, m_Width, m_Height)},
				              terrainColor,
				              m_Region == Region::DwarfMine || m_Region == Region::DwarfCity
				                  ? Material::Stone
				                  : Material::Grass);
			    });
			for (int y = 3; y < 16; y += 5)
				for (int x = 3; x < 16; x += 5)
					ModelActor(border.AddChild("Tree/" + std::to_string(y * 16 + x),
					                           {static_cast<float>(x), static_cast<float>(y)}),
					           LevelModel::Tree);
		}
	for (size_t i = 0; i < m_Enemies.size(); ++i)
	{
		ModelActor(m_Enemies[i].actor, EnemyModel(m_Enemies[i].kind), static_cast<int>(i));
		m_Enemies[i].actor.SetUpdate(
		    [this, i](Actor&, float dt)
		    {
			    UpdateEnemy(m_Enemies[i], dt);
		    });
	}
	BuildDwarfScene();
	BuildDragonScene();
	m_PlayerActor.SetUpdate(
	    [this](Actor&, float dt)
	    {
		    if (m_Input)
			    UpdatePlayer(dt);
	    });
	m_PlayerActor.SetDraw(
	    [this](Actor& actor, Renderer& r)
	    {
		    Point s = Project(actor.WorldPosition(), m_Width, m_Height);
		    Color coat = m_HitCooldown > 0 ? Color{1, .30f, .25f, 1} : Color{.35f, .49f, .67f, 1};
		    float cadence = m_Input && m_Input[GameInput::Sprint] ? 20.f : 10.f;
		    r.Character(
		        s.x, s.y, m_Facing, m_Walking ? static_cast<int>(m_Time * cadence) % 8 : 8, coat);
	    });
	Actor& shadow = m_PlayerActor.AddChild("Shadow");
	shadow.SetLayer(ActorLayer::Shadow);
	shadow.SetDraw(
	    [this](Actor& actor, Renderer& r)
	    {
		    Point s = Project(actor.WorldPosition(), m_Width, m_Height);
		    r.SoftEllipse(s.x + 8, s.y + 3, 23, 10, {.025f, .035f, .03f, .32f});
	    });
	m_PlayerActor.AddChild("Weapon").SetDraw(
	    [this](Actor& actor, Renderer& r)
	    {
		    if (m_Swing <= 0)
			    return;
		    Point s = Project(actor.WorldPosition(), m_Width, m_Height);
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
	           (m_Region == Region::DwarfCity || m_Region == Region::DragonRoost ? L" · 대화 지역"
	            : WorldTravel::IsPassage(m_Region) ? L" · 위험한 교역길"
	                                               : L" · 탐험 지역"),
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
	       m_Region == Region::DragonRoost ? L"고룡에게 E로 대화 · 시험은 주황색 바닥 예고 회피"
	       : m_Region == Region::DwarfCity ? L"평화 구역 · 기록관과 대장장이에게 E로 대화"
	       : m_Level >= 3 && m_Shards > 0
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
	       L"WASD 이동 · Shift 달리기 · Space 공격 · E 전리품/출입구 상호작용",
	       Cream);
	r.Text(30,
	       static_cast<float>(height - 49),
	       L"I 가방/스탯 · J 여행 일지 · M 지도 · P 후처리 비교 · Esc 일시정지",
	       Cream);
	auto route = m_Journal.RouteToNext(m_Region);
	r.Rect(15, 167, width - 30.f, 63, {.025f, .055f, .055f, .94f});
	r.Text(30, 174, m_Journal.Objective(), Gold);
	if (route.size() > 1)
		r.Text(30,
		       201,
		       std::wstring(L"다음 출입구: ") + WorldTravel::Name(route[1]) + L"   [J] 경로와 기록",
		       Cream);
	else if (m_Journal.NextDestination() == m_Region)
		r.Text(30, 201, L"표시된 주민·기록대·이정표에서 E · 지도에서 노란 점 확인", Cream);
	if (m_MessageTime > 0)
		r.Text(30, 242, m_Message, Gold);
	int nearbyExit = NearbyExit();
	if (nearbyExit >= 0 && !m_Map && !m_Stats && !paused && m_Hp > 0)
		r.Text(
		    30,
		    height - 120.f,
		    std::wstring(L"[E] ") +
		        WorldTravel::Name(WorldTravel::Exits(m_Region)[nearbyExit].destination) +
		        (m_Journal.CanTravel(m_Region, WorldTravel::Exits(m_Region)[nearbyExit].destination)
		             ? L" 이동"
		             : L" · 봉인된 길"),
		    Gold);
	if (((m_StoryContact && Distance(m_Player, m_StoryContact->WorldPosition()) < 2.5f) ||
	     (m_Smith && Distance(m_Player, m_Smith->WorldPosition()) < 2.5f)) &&
	    !m_Map && !m_Stats && !paused && !IsStoryOpen())
		r.Text(30, height - 120.f, L"[E] 이야기 · 대화/기록 조사", Gold);
	if (m_DragonTrace && Distance(m_Player, m_DragonTrace->WorldPosition()) < 2.5f &&
	    !IsStoryOpen() && !m_Map && !m_Stats && !paused)
		r.Text(30, height - 120.f, L"[E] 벗겨진 비늘과 경계문 조사", Gold);
	if (m_TrialWaves > 0)
		r.Text(30,
		       274,
		       L"회피 시험 · 남은 울림 " + std::to_wstring(m_TrialWaves) +
		           L" · 바닥 예고에서 벗어나세요",
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
			Color c = m_Tiles[i] == Tile::Ground && OnTravelRoad(Center(i))
			              ? Color{.60f, .51f, .34f, 1}
			          : m_Tiles[i] == Tile::Ground ? Color{.36f, .43f, .27f, 1}
			          : m_Tiles[i] == Tile::Water  ? Color{.13f, .35f, .45f, 1}
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
			bool target =
			    route.size() > 1 && WorldTravel::Exits(m_Region)[i].destination == route[1];
			bool open = m_Journal.CanTravel(m_Region, WorldTravel::Exits(m_Region)[i].destination);
			mark(position,
			     !open    ? Color{.7f, .4f, .2f, 1}
			     : target ? Gold
			              : Color{.3f, 1, .85f, 1});
			float labelX = std::min(left + position.x / 2 * scale, left + (Columns - 24) * scale);
			r.Text(labelX,
			       top + position.y / 2 * scale + 8,
			       std::wstring(open ? L"" : L"[봉인] ") +
			           WorldTravel::Name(WorldTravel::Exits(m_Region)[i].destination),
			       Cream);
		}
		if (m_SurveyStone)
			mark(m_SurveyStone->WorldPosition(),
			     m_Journal.NextDestination() == m_Region &&
			             Distance(StoryTarget(), WorldTravel::SurveyStone) < .1f
			         ? Gold
			         : Color{.8f, .7f, .4f, 1});
		if (m_StoryContact)
		{
			bool target = m_Journal.NextDestination() == m_Region &&
			              Distance(StoryTarget(), m_StoryContact->WorldPosition()) < .1f;
			mark(m_StoryContact->WorldPosition(), target ? Gold : Color{.75f, .55f, 1, 1});
			r.Text(left + m_StoryContact->WorldPosition().x / 2 * scale - 65,
			       top + m_StoryContact->WorldPosition().y / 2 * scale - 28,
			       m_Region == Region::Highlands     ? L"길잡이 세라"
			       : m_Region == Region::DragonRoost ? L"고룡 아르켄"
			       : m_Region == Region::DwarfMine   ? L"작업 기록대"
			       : m_Region == Region::DwarfCity   ? L"기록관 브린"
			                                         : L"파수꾼 토르빅",
			       Cream);
		}
		if (m_DragonTrace)
		{
			mark(m_DragonTrace->WorldPosition(),
			     m_Journal.DragonProgress() == QuestJournal::DragonStage::Trace
			         ? Gold
			         : Color{.8f, .7f, .4f, 1});
			r.Text(
			    left + 49.f / 2 * scale - 90, top + 41.f / 2 * scale + 10, L"비늘과 경계문", Cream);
		}
		mark(m_Player, {1, 1, 1, 1});
		r.Text(left,
		       top - 30,
		       L"M 닫기 · 노랑: 다음 목표 / 청록: 출입구 / 황토: 이정표 / 빨강: 적",
		       Cream);
	}
	if (m_Stats)
	{
		float x = width * .5f - 300, y = height * .5f - 180;
		r.Rect(x, y, 600, 360, {.025f, .055f, .055f, .98f});
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
		r.Text(x + 22,
		       y + 223,
		       L"야외 재료 " + std::to_wstring(m_Materials[0]) + L" · 광맥 광석 " +
		           std::to_wstring(m_Materials[1]) + L" · 수호체 부품 " +
		           std::to_wstring(m_Materials[2]),
		       Gold);
		r.Text(x + 22,
		       y + 260,
		       L"능선 비늘 " + std::to_wstring(m_Materials[3]) + L" · 바람 결정 " +
		           std::to_wstring(m_Materials[4]) + L" · 열맥 잔재 " +
		           std::to_wstring(m_Materials[5]),
		       Gold);
		r.Text(x + 22, y + 300, L"전리품과 성장 기록은 이번 실행 동안 유지됩니다.", Cream);
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

	if (IsStoryOpen() && !paused)
	{
		float panelWidth = std::min(1000.f, width - 70.f), left = (width - panelWidth) * .5f;
		float top = height - 320.f;
		r.Rect(left, top, panelWidth, 210, {.035f, .065f, .065f, .98f});
		r.Text(left + 22,
		       top + 15,
		       m_Region == Region::Highlands || m_Region == Region::DragonRoost
		           ? L"바람이 기억하는 경계"
		           : L"돌에 새긴 귀환 약속",
		       Gold);
		if (m_ChoosingDragon)
		{
			r.Text(left + 22, top + 55, L"[1] 협상 · 서식지 보존 약속, 바람길 개방", Cream);
			r.Text(
			    left + 22, top + 88, L"[2] 회피 시험 · 세 번의 울림 회피 후 기억과 바람길", Cream);
			r.Text(left + 22,
			       top + 121,
			       L"[3] 거리 두기 · 경고만 기록, 바람길 닫힘, 고원 길 유지",
			       Cream);
			r.Text(left + 22,
			       top + 170,
			       L"접근 방식은 변경할 수 없습니다. 시험 실패 시 E로 재시도합니다.",
			       Gold);
		}
		else if (m_ChoosingDwarf)
		{
			r.Text(left + 22, top + 55, L"[1] 감시 아래 교역 재개 · 분지 지름길 개방", Cream);
			r.Text(left + 22,
			       top + 91,
			       L"[2] 봉인 유지 · 기존 산길 이용, 기록과 안전 점검 우선",
			       Cream);
			r.Text(left + 22,
			       top + 145,
			       L"선택은 이번 여행에 남습니다. 두 선택 모두 다음 모험으로 이어집니다.",
			       Gold);
		}
		else
		{
			const auto& line = m_Dialogue[m_DialoguePage];
			int columns = std::max(1, static_cast<int>((panelWidth - 44) / 19));
			for (size_t i = 0; i < line.size(); i += columns)
				r.Text(left + 22,
				       top + 53 + static_cast<float>(i / columns) * 27,
				       line.substr(i, columns),
				       Cream);
			r.Text(left + 22,
			       top + 170,
			       L"[E / Space] 다음 · " + std::to_wstring(m_DialoguePage + 1) + L" / " +
			           std::to_wstring(m_Dialogue.size()),
			       Gold);
		}
	}
	m_Journal.Draw(r, width, height, m_Region);
	r.Flush();
}
