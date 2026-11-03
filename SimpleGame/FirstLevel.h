#pragma once
#include "LevelModels.h"
#include "SceneGraph.h"
#include "WorldTravel.h"
#include <array>
#include <cstdint>
#include <string>

struct MonitorSnapshot;
class QuestJournal;

class FirstLevel
{
  public:
	using Region = WorldTravel::Region;
	using Exit = WorldTravel::Region;

	explicit FirstLevel(QuestJournal& journal);

	SceneGraph& Scene()
	{
		return m_Scene;
	}

	void Begin(uint32_t seed,
	           bool resetProgress = false,
	           Region region = Region::Passage,
	           Region from = Region::Village);
	bool Update(float seconds, const bool* keys);
	bool Key(unsigned char key);

	bool CanOpenJournal() const
	{
		return !m_Map && !m_Stats && m_Dialogue.empty() && !m_ChoosingDwarf && m_Hp > 0;
	}

	bool IsStoryOpen() const
	{
		return !m_Dialogue.empty() || m_ChoosingDwarf;
	}

	Exit RequestedExit() const
	{
		return m_RequestedExit;
	}

	Region CurrentRegion() const
	{
		return m_Region;
	}

	void Draw(Renderer& renderer, int width, int height, bool postprocess, bool paused);
	void CaptureMonitorState(MonitorSnapshot& snapshot) const;

  private:
	static constexpr int Columns = WorldTravel::Columns;
	static constexpr int Rows = WorldTravel::Rows;
	static constexpr float TileSize = 2.f;
	enum class Tile : unsigned char
	{
		Ground,
		Tree,
		Rock,
		Water
	};

	enum class EnemyKind
	{
		Slime,
		Bandit,
		Archer,
		Priest,
		Wolf,
		Beetle,
		Guardian,
		Count
	};

	struct EnemyStats
	{
		int hp, damage, experience, coins;
		float speed, range, windup, cooldown;
		Color color;
	};

	static const EnemyStats& Stats(EnemyKind kind);

	struct Enemy
	{
		Actor& actor;
		Point& position;
		Point home;
		EnemyKind kind;
		Point attackTarget = {0, 0};
		float supportCooldown = 3, glow = 0, charge = 0;
		Point chargeDirection = {};
		int facing = 1;
		bool moving = false;
		int hp = 24;
		float cooldown = 1, windup = 0, respawn = 0, flash = 0;

		Enemy(Actor& node, Point origin, EnemyKind type)
		    : actor(node), position(node.Position()), home(origin), kind(type), hp(Stats(type).hp)
		{
		}
	};

	struct Drop
	{
		Actor* actor;
		int coins, shards;
		int material = -1;
	};

	enum class StoryAction
	{
		None,
		Start,
		Record,
		Council
	};

	QuestJournal& m_Journal;
	SceneGraph m_Scene;
	Actor& m_PlayerActor;
	Actor& m_StaticActors;
	Actor& m_EnemyActors;
	Actor& m_DropActors;
	Point& m_Player;
	const bool* m_Input = nullptr;
	int m_Width = 1280, m_Height = 800;
	bool m_Paused = false;
	std::array<Tile, Columns * Rows> m_Tiles;
	std::array<int, Columns * Rows> m_Distance;
	std::vector<Enemy> m_Enemies;
	std::vector<Drop> m_Drops;
	std::vector<Actor*> m_Exits;
	Actor* m_SurveyStone = nullptr;
	Actor* m_StoryContact = nullptr;
	Actor* m_Smith = nullptr;
	std::vector<std::wstring> m_Dialogue;
	size_t m_DialoguePage = 0;
	StoryAction m_StoryAction = StoryAction::None;
	bool m_ChoosingDwarf = false;
	Region m_Region = Region::Passage;
	Exit m_RequestedExit = Exit::None;
	WorldTravel::ReturnTrail m_ReturnTrail;
	bool m_ReturnedFromEdge = false;
	LevelModels m_Models;
	Point m_Camera = WorldTravel::NearGate, m_Aim = {1, 0};
	int m_Level = 1, m_Experience = 0, m_Hp = 100, m_Coins = 0, m_Shards = 0;
	std::array<int, 3> m_Materials = {};
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
	bool NeighborPathsConnected(int cell) const;
	int Cell(Point position) const;
	Point Center(int cell) const;
	bool Walkable(Point position) const;
	bool ClearLine(Point from, Point to) const;
	bool SafeExit(Point position, float margin = 9) const;
	bool OnTravelRoad(Point position) const;
	int NearbyExit() const;
	void Move(Point& position, Point delta);
	void RebuildFlow();
	void Step(float dt, const bool* keys);
	void UpdatePlayer(float dt);
	void UpdateEnemy(Enemy& enemy, float dt);
	void BuildScene();
	void BuildDwarfScene();
	void InteractStory();
	void FinishStory();
	Point StoryTarget() const;
	LevelModel EnemyModel(EnemyKind kind) const;
	void ModelActor(Actor& actor, LevelModel model, int enemy = -1);
	void DrawHud(Renderer& renderer, int width, int height, bool paused);
	void Attack();
	void Pickup();
	void GainExperience(int amount);
	void Message(const std::wstring& text);
	Point Project(Point point, int width, int height) const;
};
