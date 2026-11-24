#include "../SimpleGame/stdafx.h"
#include "../SimpleGame/SceneGraph.h"
#include "../SimpleGame/LevelModels.h"
#include "../SimpleGame/WorldTravel.h"
#include "../SimpleGame/QuestJournal.h"
#include "../SimpleGame/MonitorState.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <queue>
#define private public
#include "../SimpleGame/FirstLevel.h"
#undef private
#include "../SimpleGame/FirstLevel.cpp"
#include "../SimpleGame/FirstLevelDragon.cpp"
#include <cassert>

int main()
{
	using Region = FirstLevel::Region;
	using Gear = FirstLevel::GearState;
	QuestJournal journal;
	FirstLevel level(journal);
	level.Begin(42, true, Region::Passage);
	level.m_Hp = 25;
	level.GainExperience(90);
	assert(level.m_Level == 3 && level.MaxHp() == 130 && level.m_Hp == 25);

	level.m_Materials[0] = 6;
	level.Key('1');
	assert(level.m_Gear[0] == Gear::Unmade);
	level.Key('i');
	level.Key('1');
	level.Key('2');
	assert(level.m_Gear[0] == Gear::Equipped && level.m_Materials[0] == 3 &&
	       level.AttackPower() == 24);
	assert(level.m_Gear[1] == Gear::Unmade && level.Defense() == 2);
	level.Key('1');
	assert(level.m_Gear[0] == Gear::Stored && level.AttackPower() == 20);
	level.Key('1');
	++level.m_Materials[0];
	level.Key('2');
	assert(level.m_Gear[0] == Gear::Equipped && level.m_Gear[1] == Gear::Equipped &&
	       level.m_Materials[0] == 0 && level.Defense() == 4);

	level.m_Coins = 5;
	level.Key('h');
	assert(level.m_Coins == 5 && level.m_Hp == 25);
	level.m_Coins = 18;
	level.Key('h');
	assert(level.m_Coins == 12 && level.m_Hp == 64);
	level.m_Hp = 129;
	level.Key('h');
	level.Key('h');
	assert(level.m_Coins == 6 && level.m_Hp == 130);
	level.Key('m');
	assert(level.m_Map && !level.m_Stats);
	level.Key('1');
	assert(level.m_Gear[0] == Gear::Equipped);
	level.Key('i');
	assert(!level.m_Map && level.m_Stats);
	level.Key('i');

	level.m_Hp = 37;
	level.Begin(43, false, Region::Basin, Region::Passage);
	assert(level.m_Hp == 37 && level.m_Coins == 6 && level.AttackPower() == 24 &&
	       level.Defense() == 4);
	assert(journal.Investigate(Region::Basin));
	const auto clues = journal.Lines(1, Region::Basin);
	level.m_Shards = 2;
	level.m_Player = WorldTravel::FarGate;
	level.m_Coins = 20;
	level.m_Hp = 0;
	level.Key('r');
	level.Key('r');
	assert(level.m_Hp == 65 && level.m_Coins == 5 && level.m_RevivalDebt == 0);
	assert(level.Walkable(level.m_Player) && level.SafeExit(level.m_Player) &&
	       Distance(level.m_Player, WorldTravel::NearGate) < .001f &&
	       Distance(level.m_Player, level.m_Camera) < .001f);
	assert(level.CurrentRegion() == Region::Basin && level.m_Seed == 43 &&
	       level.m_Experience == 90 && level.m_Shards == 2 && level.AttackPower() == 24 &&
	       journal.Lines(1, Region::Basin) == clues);

	level.m_Hp = 0;
	level.Key('r');
	assert(level.m_Coins == 0 && level.m_RevivalDebt == 10 && level.m_Hp == 65);
	level.Begin(44, false, Region::Passage, Region::Basin);
	assert(level.m_RevivalDebt == 10 && level.m_Hp == 65);
	level.m_Player = {27, 23};
	for (int coins : {7, 7})
	{
		Actor& loot = level.m_DropActors.AddChild("TestLoot", level.m_Player);
		level.m_Drops.push_back({&loot, coins, 1, 0});
		assert(!level.Key('e'));
	}
	assert(level.m_RevivalDebt == 0 && level.m_Coins == 4 && level.m_Shards == 4 &&
	       level.m_Materials[0] == 2);
	level.m_Hp = 0;
	level.Key('r');
	assert(level.m_RevivalDebt == 11);
	level.Reset();
	assert(level.m_RevivalDebt == 0 && level.m_Gear[0] == Gear::Unmade &&
	       level.m_Gear[1] == Gear::Unmade && level.m_Experience == 0 && level.m_Hp == 100);
}
