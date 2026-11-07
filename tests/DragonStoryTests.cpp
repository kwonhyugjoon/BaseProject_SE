#include "../SimpleGame/stdafx.h"
#include "../SimpleGame/SceneGraph.h"
#include "../SimpleGame/LevelModels.h"
#include "../SimpleGame/WorldTravel.h"
#include "../SimpleGame/QuestJournal.h"
#include "../SimpleGame/MonitorState.h"
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

namespace
{
	using Region = WorldTravel::Region;
	using Stage = QuestJournal::DragonStage;
	using Kind = FirstLevel::EnemyKind;

	void FinishPages(FirstLevel& level)
	{
		int pages = 0;
		while (!level.m_Dialogue.empty())
		{
			assert(++pages < 10);
			level.Key('e');
		}
	}

	void FinishTutorial(QuestJournal& journal, int choice)
	{
		for (auto stage : {QuestJournal::Stage::Elder,
		                   QuestJournal::Stage::Scout,
		                   QuestJournal::Stage::Rune,
		                   QuestJournal::Stage::Fragment,
		                   QuestJournal::Stage::Fisher})
			assert(journal.CompleteStep(stage));
		assert(journal.ChooseLake(choice));
		assert(journal.CompleteStep(QuestJournal::Stage::Return));
	}
}

int main()
{
	for (int choice : {1, 2, 3})
	{
		QuestJournal journal;
		FirstLevel level(journal);
		assert(!journal.StartDragonStory() && !journal.ReadDragonTrace() && !journal.HearDragon() &&
		       !journal.ChooseDragon(choice) && !journal.CompleteDragonTrial() &&
		       !journal.ReadDragonMemory());
		FinishTutorial(journal, choice == 2 ? 2 : 1);
		level.Begin(42, true, Region::Basin);
		level.m_Player = {49, 57};
		assert(level.Key('e') && level.RequestedExit() == Region::Highlands);
		level.Begin(43, false, Region::Highlands, Region::Basin);
		level.m_Hp = 37;
		level.m_Player = WorldTravel::FarGate;
		assert(!level.Key('e'));
		level.m_Player = level.m_DragonTrace->WorldPosition();
		level.Key('e');
		FinishPages(level);
		assert(journal.DragonProgress() == Stage::Meeting);
		level.m_Player = level.m_StoryContact->WorldPosition();
		level.Key('e');
		assert(!level.CanOpenJournal() && journal.DragonProgress() == Stage::Meeting);
		bool keys[256] = {};
		keys['d'] = keys[' '] = true;
		Point before = level.m_Player;
		assert(!level.Update(.25f, keys) && Distance(before, level.m_Player) == 0);
		FinishPages(level);
		assert(journal.DragonProgress() == Stage::Trace &&
		       journal.NextDestination() == Region::Highlands);
		level.m_Player = level.m_DragonTrace->WorldPosition();
		level.Key('e');
		assert(!journal.CanTravel(Region::Highlands, Region::DragonRoost));
		FinishPages(level);
		assert(journal.DragonProgress() == Stage::Audience);
		assert(journal.NextDestination() == Region::DragonRoost);
		auto route = journal.RouteToNext(Region::Village);
		assert(route.size() == 5 && route[3] == Region::Highlands);
		level.m_Player = WorldTravel::FarGate;
		assert(level.Key('e'));
		level.Begin(44, false, Region::DragonRoost, Region::Highlands);
		assert(level.m_Hp == 37 && level.m_Enemies.empty());
		level.m_Player = level.m_StoryContact->WorldPosition();
		level.Key('e');
		FinishPages(level);
		assert(level.m_ChoosingDragon && journal.DragonProgress() == Stage::Choice);
		level.Key('4');
		assert(journal.DragonApproach() == 0 && level.m_ChoosingDragon);
		int xp = level.m_Experience;
		level.Key(static_cast<unsigned char>('0' + choice));
		assert(journal.DragonChoice() == 0 && !journal.ChooseDragon(1));
		if (choice == 2)
		{
			assert(journal.DragonProgress() == Stage::Trial && level.m_TrialWaves == 3);
			level.UpdateDragonTrial(1.6f); // Standing in the announced area fails the trial.
			assert(level.m_TrialWaves == 0 && level.m_Hp == 25 &&
			       journal.DragonProgress() == Stage::Trial);
			level.Key('e');
			assert(level.m_TrialWaves == 3);
			level.Begin(45, false, Region::Highlands);
			level.Begin(46, false, Region::DragonRoost, Region::Highlands);
			assert(level.m_TrialWaves == 0 && journal.DragonProgress() == Stage::Trial);
			level.m_Player = WorldTravel::SurveyStone;
			level.Key('e');
			level.m_Player = {60, 41};
			level.UpdateDragonTrial(.01f); // Leaving the arena cancels, without completing it.
			assert(level.m_TrialWaves == 0 && journal.DragonProgress() == Stage::Trial);
			level.m_Player = WorldTravel::SurveyStone;
			level.Key('e');
			for (int wave = 0; wave < 3; ++wave)
			{
				level.m_Player = wave % 2 ? WorldTravel::SurveyStone : Point{53, 41};
				level.UpdateDragonTrial(1.6f);
			}
			assert(journal.DragonProgress() == Stage::Memory && level.m_TrialWaves == 0 &&
			       level.m_Hp == 25);
			level.m_Player = WorldTravel::SurveyStone;
			level.Key('e');
		}
		assert(!level.m_Dialogue.empty() && level.m_Experience == xp);
		FinishPages(level);
		assert(journal.DragonProgress() == Stage::Complete && journal.DragonChoice() == choice);
		assert(level.m_Experience == xp + 75 && level.m_Hp == (choice == 2 ? 25 : 37));
		assert(journal.DwarfProgress() == QuestJournal::DwarfStage::Meeting);
		assert(journal.CanTravel(Region::DragonRoost, Region::StoneRidge) == (choice != 3));
		assert(journal.CanTravel(Region::StoneRidge, Region::DragonRoost) == (choice != 3));
		level.m_Player = WorldTravel::FarGate;
		assert(level.Key('e') == (choice != 3));
		level.m_Player = WorldTravel::NearGate;
		assert(level.Key('e')); // All choices retain the return route.
		level.m_Player = WorldTravel::SurveyStone;
		level.Key('e');
		FinishPages(level);
		assert(level.m_Experience == xp + 75);
		MonitorSnapshot snapshot;
		snapshot.scene = "dragon_roost";
		level.CaptureMonitorState(snapshot);
		assert(snapshot.npcs.size() == 5 && snapshot.npcs[4].visible &&
		       snapshot.villages[1].population == 2);
		assert(snapshot.villages[1].mood ==
		       (choice == 3 ? "서로 거리를 지킴" : "경계의 기억을 나눔"));
		auto clues = journal.Lines(1, Region::DragonRoost);
		level.m_Hp = 0;
		level.Key('r');
		assert(journal.DragonChoice() == choice && journal.Lines(1, Region::DragonRoost) == clues);
		journal.Reset();
		assert(journal.DragonProgress() == Stage::Meeting && journal.DragonApproach() == 0);
	}

	QuestJournal journal;
	FirstLevel level(journal);
	level.Begin(50, true, Region::Highlands);
	MonitorSnapshot snapshot;
	level.CaptureMonitorState(snapshot);
	for (int i = 7; i < 10; ++i)
		assert(snapshot.mobs[i].total == 16 && snapshot.mobs[i].alive == 16);
	for (auto kind : {Kind::ScaleLizard, Kind::WindSprite, Kind::Emberling})
	{
		for (auto& foe : level.m_Enemies)
			if (foe.kind == kind)
			{
				foe.position = {23.8f, 23.8f};
				foe.hp = 1;
				level.Attack();
				assert(foe.hp == 0);
				break;
			}
	}
	level.Pickup();
	assert(level.m_Materials[3] == 1 && level.m_Materials[4] == 1 && level.m_Materials[5] == 1);
	level.Begin(51, false, Region::Highlands);
	assert(level.m_Materials[3] == 1 && level.m_Materials[4] == 1 && level.m_Materials[5] == 1);
	level.m_Tiles.fill(FirstLevel::Tile::Ground);
	level.m_Player = {41, 27};
	auto& foe = level.m_Enemies[0];
	foe.kind = Kind::ScaleLizard;
	foe.position = {38, 27};
	foe.cooldown = 0;
	level.UpdateEnemy(foe, .01f);
	level.m_Player = {41, 31};
	level.UpdateEnemy(foe, 1.1f);
	assert(foe.charge > 0 && foe.chargeDirection.x > .99f);
	level.UpdateEnemy(foe, .1f);
	assert(foe.position.x > 38 && foe.position.y == 27);
	foe.charge = 0;
	foe.kind = Kind::WindSprite;
	foe.position = {38, 27};
	foe.cooldown = 0;
	level.m_Player = {41, 27};
	level.m_HitCooldown = 0;
	level.UpdateEnemy(foe, .01f);
	int hp = level.m_Hp;
	level.UpdateEnemy(foe, 1.2f);
	assert(level.m_Player.x > 42 && level.m_Hp < hp);
	foe.cooldown = 0;
	level.m_HitCooldown = 0;
	level.m_Player = {41, 27};
	level.UpdateEnemy(foe, .01f);
	level.m_Player = {41, 31};
	hp = level.m_Hp;
	level.UpdateEnemy(foe, 1.2f);
	assert(level.m_Hp == hp);
	foe.kind = Kind::Emberling;
	foe.cooldown = 0;
	level.m_Player = {41, 27};
	level.UpdateEnemy(foe, .01f);
	level.UpdateEnemy(foe, 1.5f);
	assert(foe.heat == 3);
	level.m_Player = {41, 31};
	level.UpdateEnemy(foe, .1f);
	assert(level.m_Hp == hp);
	level.m_Player = {41, 27};
	level.UpdateEnemy(foe, .1f);
	assert(level.m_Hp < hp);
	level.UpdateEnemy(foe, 3.1f);
	assert(foe.heat == 0);
	puts(
	    "PASS: independent dragon branch, three approaches, trial retry/dodge, history, reciprocal shortcut, themed loot, charge, gust and persistent heat.");
}
