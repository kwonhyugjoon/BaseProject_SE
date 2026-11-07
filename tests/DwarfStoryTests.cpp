#include "../SimpleGame/stdafx.h"
#include "../SimpleGame/SceneGraph.h"
#include "../SimpleGame/LevelModels.h"
#include "../SimpleGame/WorldTravel.h"
#include "../SimpleGame/QuestJournal.h"
#include "../SimpleGame/MonitorState.h"
#include "../SimpleGame/AssetCache.h"
#include <cstring>
#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <iostream>
#define private public
#include "../SimpleGame/FirstLevel.h"
#undef private
#include "../SimpleGame/FirstLevel.cpp"
#include "../SimpleGame/FirstLevelDragon.cpp"
#include <cassert>
#include <cstdio>
#include <set>

namespace
{
	using Region = WorldTravel::Region;
	using DwarfStage = QuestJournal::DwarfStage;

	void FinishPages(FirstLevel& level)
	{
		int pages = 0;
		while (!level.m_Dialogue.empty())
		{
			assert(++pages < 20);
			level.Key('e');
		}
	}

	void FinishTutorial(QuestJournal& journal)
	{
		for (auto step : {QuestJournal::Stage::Elder,
		                  QuestJournal::Stage::Scout,
		                  QuestJournal::Stage::Rune,
		                  QuestJournal::Stage::Fragment,
		                  QuestJournal::Stage::Fisher})
			assert(journal.CompleteStep(step));
		assert(journal.ChooseLake(2));
		assert(journal.CompleteStep(QuestJournal::Stage::Return));
		for (Region region : {Region::Basin, Region::StoneRidge, Region::MistShore})
		{
			journal.Visit(region);
			assert(journal.Investigate(region));
		}
	}
}

int main()
{
	for (int choice : {1, 2})
	{
		QuestJournal journal;
		FirstLevel level(journal);
		assert(!journal.ReadDwarfRecord() && !journal.HearDwarfCouncil() &&
		       !journal.ChooseDwarf(1));
		FinishTutorial(journal);
		level.Begin(42, true, Region::StoneRidge);
		level.m_Player = level.m_StoryContact->WorldPosition();
		level.Key('e');
		assert(level.IsStoryOpen() && !level.CanOpenJournal());
		assert(journal.DwarfProgress() == DwarfStage::Meeting);
		Point before = level.m_Player;
		bool input[256] = {};
		input['d'] = input[' '] = true;
		assert(!level.Update(.25f, input));
		assert(Distance(before, level.m_Player) == 0);
		level.Key('1');
		assert(journal.DwarfProgress() == DwarfStage::Meeting);
		FinishPages(level);
		assert(journal.DwarfProgress() == DwarfStage::Record);
		assert(journal.NextDestination() == Region::DwarfMine);
		level.m_Player = WorldTravel::FarGate;
		assert(level.Key('e') && level.RequestedExit() == Region::DwarfMine);
		journal.RecordTravel(Region::StoneRidge, Region::DwarfMine);
		level.Begin(43, false, Region::DwarfMine, Region::StoneRidge);
		level.m_Hp = 37;
		level.m_Player = WorldTravel::FarGate;
		assert(!level.Key('e')); // The city requires a record, never an enemy-kill quota.
		level.m_Player = {73, 23};
		assert(!level.Key('e')); // Shortcut is sealed until an actual choice.
		level.m_Player = level.m_StoryContact->WorldPosition();
		level.Key('e');
		assert(!journal.CanTravel(Region::DwarfMine, Region::DwarfCity));
		FinishPages(level);
		assert(journal.DwarfProgress() == DwarfStage::Report);
		assert(journal.CanTravel(Region::DwarfMine, Region::DwarfCity));
		level.m_Player = WorldTravel::FarGate;
		assert(level.Key('e') && level.RequestedExit() == Region::DwarfCity);
		journal.RecordTravel(Region::DwarfMine, Region::DwarfCity);
		level.Begin(44, false, Region::DwarfCity, Region::DwarfMine);
		assert(level.m_Hp == 37 && level.m_Enemies.empty() && level.m_Smith);
		level.m_Player = level.m_StoryContact->WorldPosition();
		level.Key('e');
		FinishPages(level);
		assert(level.m_ChoosingDwarf && journal.DwarfProgress() == DwarfStage::Choice);
		assert(!level.CanOpenJournal());
		level.Key('3');
		assert(level.m_ChoosingDwarf && journal.DwarfChoice() == 0);
		int xp = level.m_Experience;
		level.Key(static_cast<unsigned char>('0' + choice));
		assert(journal.DwarfProgress() == DwarfStage::Complete && journal.DwarfChoice() == choice);
		assert(level.m_Experience == xp + 75 && level.m_Hp == 37 && !level.IsStoryOpen());
		level.Key(static_cast<unsigned char>('0' + choice));
		assert(level.m_Experience == xp + 75 && !journal.ChooseDwarf(3 - choice));
		assert(journal.CanTravel(Region::Basin, Region::DwarfMine) == (choice == 1));
		assert(journal.CanTravel(Region::DwarfMine, Region::Basin) == (choice == 1));
		assert(journal.NextDestination() == Region::Highlands);
		auto clues = journal.Lines(1, Region::DwarfCity);
		level.Begin(45, false, Region::DwarfMine, Region::DwarfCity);
		assert(journal.Lines(1, Region::DwarfMine) == clues && level.m_Hp == 37);
		level.m_Player = {73, 23};
		assert(level.Key('e') == (choice == 1));
		level.Begin(46, false, Region::Basin, Region::EchoHall);
		level.m_Player = {73, 23};
		assert(level.Key('e') == (choice == 1));
		level.m_Hp = 0;
		level.Key('r');
		assert(journal.Lines(1, Region::Basin) == clues && journal.LakeChoice() == 2);
		level.Begin(47, false, Region::DwarfCity, Region::DwarfMine);
		level.m_StoryContact->SetWorldPosition({50, 43});
		MonitorSnapshot snapshot;
		snapshot.scene = "dwarf_city";
		level.CaptureMonitorState(snapshot);
		assert(snapshot.npcs.size() == 5 && snapshot.villages.size() == 2);
		assert(snapshot.npcs[1].x == 50 && snapshot.npcs[1].y == 43 && snapshot.npcs[1].visible);
		assert(snapshot.villages[0].population == 3);
		assert(snapshot.villages[0].mood ==
		       (choice == 1 ? "교역 재개를 준비함" : "안전 점검에 집중함"));
		journal.Reset();
		assert(journal.DwarfProgress() == DwarfStage::Meeting && journal.DwarfChoice() == 0);
		assert(!journal.CanTravel(Region::Basin, Region::DwarfMine));
	}

	QuestJournal journal;
	FirstLevel level(journal);
	level.Begin(100, true, Region::DwarfMine);
	std::set<FirstLevel::EnemyKind> kinds;
	for (const auto& enemy : level.m_Enemies)
		kinds.insert(enemy.kind);
	assert(kinds.size() == 2 && kinds.count(FirstLevel::EnemyKind::Beetle) &&
	       kinds.count(FirstLevel::EnemyKind::Guardian));
	const auto& beetleModel = level.m_Models.Get(LevelModel::Beetle);
	const auto& guardianModel = level.m_Models.Get(LevelModel::Guardian);
	assert(!beetleModel.empty() && !guardianModel.empty() &&
	       beetleModel.size() != guardianModel.size());

	std::vector<unsigned char> cachedData;
	assert(LoadAssetCache(L"level-models-v3.mesh", 3, 4 * 1024 * 1024, cachedData));
	LevelModels reloaded;
	reloaded.Load();
	for (int i = 0; i < static_cast<int>(LevelModel::Count); ++i)
	{
		const auto& original = level.m_Models.Get(static_cast<LevelModel>(i));
		const auto& loaded = reloaded.Get(static_cast<LevelModel>(i));
		assert(original.size() == loaded.size());
		assert(std::memcmp(original.data(), loaded.data(), original.size() * sizeof(ModelVertex)) ==
		       0);
	}

	// Verify the actual kill -> drop -> pickup path for ore and machinery.
	for (auto kind : {FirstLevel::EnemyKind::Beetle, FirstLevel::EnemyKind::Guardian})
	{
		for (auto& enemy : level.m_Enemies)
		{
			if (enemy.kind != kind)
				continue;
			enemy.position = {WorldTravel::NearGate.x + .8f, WorldTravel::NearGate.y + .8f};
			enemy.hp = 1;
			level.Attack();
			assert(enemy.hp == 0);
			break;
		}
	}
	assert(level.m_Drops.size() == 2);
	level.Pickup();
	assert(level.m_Materials[1] == 1 && level.m_Materials[2] == 1 && level.m_Shards == 2);
	level.Begin(101, false, Region::DwarfCity);
	assert(level.m_Materials[1] == 1 && level.m_Materials[2] == 1);

	// A wolf locks its charge direction before the traveler dodges.
	level.Begin(102, true, Region::Passage);
	level.m_Tiles.fill(FirstLevel::Tile::Ground);
	level.m_Player = {41, 27};
	auto& wolf = level.m_Enemies[0];
	wolf.kind = FirstLevel::EnemyKind::Wolf;
	wolf.position = {38, 27};
	wolf.cooldown = 0;
	level.UpdateEnemy(wolf, .01f);
	assert(wolf.windup > 0 && wolf.charge == 0);
	level.m_Player = {41, 31};
	level.UpdateEnemy(wolf, 1.f);
	assert(wolf.charge > 0 && wolf.chargeDirection.x > .99f);
	int hp = level.m_Hp;
	level.UpdateEnemy(wolf, .1f);
	assert(wolf.position.x > 38 && wolf.position.y == 27 && level.m_Hp == hp);

	// The guardian slams the announced ground, so moving away avoids damage.
	level.Begin(103, true, Region::DwarfMine);
	level.m_Tiles.fill(FirstLevel::Tile::Ground);
	level.m_Player = {41, 27};
	auto& guardian = level.m_Enemies[0];
	guardian.kind = FirstLevel::EnemyKind::Guardian;
	guardian.position = {38, 27};
	guardian.cooldown = 0;
	level.UpdateEnemy(guardian, .01f);
	assert(guardian.windup > 0);
	level.m_Player = {41, 31};
	hp = level.m_Hp;
	level.UpdateEnemy(guardian, 2.f);
	assert(level.m_Hp == hp);
	level.m_Player = {41, 27};
	guardian.cooldown = 0;
	level.UpdateEnemy(guardian, .01f);
	level.UpdateEnemy(guardian, 2.f);
	assert(level.m_Hp < hp);
	puts(
	    "PASS: both dwarf branches, gated reciprocal shortcut, dialogue freeze, history/reset, themes, ore/parts, wolf dodge and guardian telegraph.");
}
