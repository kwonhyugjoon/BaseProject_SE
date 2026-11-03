#include "../SimpleGame/stdafx.h"
#include "../SimpleGame/SceneGraph.h"
#include "../SimpleGame/LevelModels.h"
#include "../SimpleGame/WorldTravel.h"
#include <array>
#include <algorithm>
#include <queue>
#include <iostream>
#include <cmath>
#define private public
#include "../SimpleGame/FirstLevel.h"
#undef private
// Compile implementation here so MSVC access-dependent symbols match this test.
#include "../SimpleGame/FirstLevel.cpp"
#include <cassert>
#include <cstdio>

int main()
{
	using Region = FirstLevel::Region;
	QuestJournal journal;
	FirstLevel level(journal);
	assert(WorldTravel::Exits(Region::Passage).size() == 2);
	assert(WorldTravel::Exits(Region::EchoHall).size() == 3);
	for (auto region : {Region::Passage,
	                    Region::Basin,
	                    Region::EchoHall,
	                    Region::StoneRidge,
	                    Region::MistShore,
	                    Region::DwarfMine,
	                    Region::DwarfCity})
		for (unsigned seed = 1; seed <= 32; ++seed)
		{
			level.Begin(seed, true, region);
			assert(level.Connected());
			for (int cell = 0; cell < level.Columns * level.Rows; ++cell)
				if (level.OnTravelRoad(level.Center(cell)))
					assert(level.m_Tiles[cell] == FirstLevel::Tile::Ground);
			for (const auto& link : WorldTravel::Exits(region))
				assert(level.ClearLine(link.position, WorldTravel::SurveyStone));
			assert(level.m_Exits.size() == WorldTravel::Exits(region).size());
			for (size_t i = 0; i < level.m_Exits.size(); ++i)
			{
				const auto& link = WorldTravel::Exits(region)[i];
				assert(level.Walkable(link.position));
				level.m_Player = link.position;
				bool open = journal.CanTravel(region, link.destination);
				assert(level.Key('e') == open);
				if (open)
					assert(level.RequestedExit() == link.destination);
				bool reciprocal = false;
				for (const auto& back : WorldTravel::Exits(link.destination))
					reciprocal = reciprocal || back.destination == region;
				assert(reciprocal);
			}
			int sectors[16] = {};
			for (const auto& enemy : level.m_Enemies)
			{
				assert(level.Walkable(enemy.home) && !level.SafeExit(enemy.home));
				++sectors[static_cast<int>(enemy.home.y) / 20 * 4 +
				          static_cast<int>(enemy.home.x) / 24];
			}
			assert(level.m_Enemies.size() == (region == Region::DwarfCity ? 0 : 48));
			for (int count : sectors)
				assert(count == (region == Region::DwarfCity ? 0 : 3));
		}
	level.m_Experience = 90;
	level.m_Level = 3;
	level.m_Hp = 37;
	level.m_Shards = 4;
	level.Begin(42, false, Region::Passage, Region::Basin);
	assert(level.m_Hp == 37 && level.m_Experience == 90 && level.m_Shards == 4);
	assert(level.Key('e') && level.RequestedExit() == FirstLevel::Exit::Basin);
	bool input[256] = {};
	for (auto region : {Region::Passage, Region::Basin, Region::EchoHall})
	{
		level.Begin(42, false, region);
		// No movement is applied: check all four boundary cases and preserved state.
		for (Point edge : {Point{3, 7}, Point{7, 3}, Point{93, 41}, Point{49, 77}})
		{
			level.m_ReturnTrail.lastSafe = WorldTravel::NearGate;
			level.m_Player = edge;
			assert(level.Update(.01f, input));
			assert(level.Walkable(level.m_Player));
			assert(level.m_Hp == 37 && level.m_Experience == 90 && level.m_Shards == 4);
			assert(level.m_Message.find(WorldTravel::IsPassage(region) ? L"샛길" : L"숲") !=
			       std::wstring::npos);
		}
	}
	level.Begin(43, false, Region::Basin);
	level.m_Player = {3, 7};
	assert(level.Update(.01f, input));
	assert(level.Walkable(level.m_Player) && Distance(level.m_Player, Camp) < .001f);
	assert(!level.Update(.01f, input));
	level.Key('m');
	level.m_Player = {3, 7};
	assert(!level.Update(.01f, input) && level.m_Player.x == 3);
	level.Key('m');
	assert(level.Update(.01f, input));
	level.m_Hp = 0;
	assert(!level.Key('e'));
	level.Key('r');
	assert(level.CurrentRegion() == Region::Basin && level.m_Experience == 0);
	puts(
	    "PASS: 224 maps, variable reciprocal exits, 48 enemies/16 sectors or peaceful city, state and edge returns.");
}
