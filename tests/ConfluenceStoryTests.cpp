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

namespace
{
	using Region = WorldTravel::Region;
	using Stage = QuestJournal::ConfluenceStage;
	const Region Contacts[] = {Region::DwarfCity, Region::Highlands, Region::MistShore};

	void FinishRegionalStory(QuestJournal& journal, Region region, int choice)
	{
		const int before = journal.CompletedRegionalStories();
		journal.Visit(region);
		switch (region)
		{
		case Region::DwarfCity:
			journal.Visit(Region::StoneRidge);
			assert(journal.Investigate(Region::StoneRidge) && journal.StartDwarfStory());
			journal.Visit(Region::DwarfMine);
			assert(journal.ReadDwarfRecord());
			assert(journal.HearDwarfCouncil() && journal.ChooseDwarf(choice));
			break;
		case Region::Highlands:
			assert(journal.StartDragonStory() && journal.ReadDragonTrace());
			journal.Visit(Region::DragonRoost);
			assert(journal.HearDragon() && journal.ChooseDragon(choice));
			assert(journal.CompletedRegionalStories() == before);
			if (choice == 2)
				assert(journal.CompleteDragonTrial());
			assert(journal.ReadDragonMemory());
			break;
		case Region::MistShore:
			assert(journal.HearCoastDream());
			journal.Visit(Region::TidalRuins);
			assert(journal.ReadTidalShrine() && journal.ChooseCoast(choice));
			break;
		default:
			assert(false);
		}
		assert(journal.CompletedRegionalStories() == before + 1);
	}

	void ReadDialogue(FirstLevel& level)
	{
		assert(level.IsStoryOpen() && level.IsSimulationPaused());
		while (level.IsStoryOpen())
			level.Key('e');
	}

	bool TryExit(FirstLevel& level, Region to)
	{
		const auto& exits = WorldTravel::Exits(level.CurrentRegion());
		const auto exit = std::find_if(exits.begin(),
		                               exits.end(),
		                               [to](const WorldTravel::ExitLink& link)
		                               {
			                               return link.destination == to;
		                               });
		assert(exit != exits.end());
		level.m_Player = exit->position;
		const bool allowed = level.Key('e');
		if (allowed)
			assert(level.RequestedExit() == to);
		return allowed;
	}
}

int main()
{
	for (Region first : Contacts)
		for (Region second : Contacts)
		{
			if (first == second)
				continue;

			QuestJournal journal;
			FirstLevel level(journal);
			for (auto stage : {QuestJournal::Stage::Elder,
			                   QuestJournal::Stage::Scout,
			                   QuestJournal::Stage::Rune,
			                   QuestJournal::Stage::Fragment,
			                   QuestJournal::Stage::Fisher})
				assert(journal.CompleteStep(stage));
			assert(journal.ChooseLake(2) && journal.CompleteStep(QuestJournal::Stage::Return));
			level.Begin(42, true, Region::Basin);
			assert(journal.ConfluenceProgress() == Stage::Locked &&
			       !TryExit(level, Region::Confluence));
			FinishRegionalStory(journal, first, first == Region::Highlands ? 3 : 2);
			assert(journal.ConfluenceProgress() == Stage::Locked &&
			       !TryExit(level, Region::Confluence));
			FinishRegionalStory(journal, second, second == Region::Highlands ? 2 : 1);
			assert(journal.ConfluenceProgress() == Stage::Meeting);
			assert(!journal.HearConfluenceBriefing());
			assert((journal.RouteToNext(Region::Basin) ==
			        std::vector<Region>{Region::Basin, Region::Confluence}));
			assert(TryExit(level, Region::Confluence));
			level.Begin(43, false, Region::Confluence, Region::Basin);
			assert(level.m_Enemies.empty() && level.Connected());
			assert(!TryExit(level, second));
			level.m_Player = level.m_SurveyStone->WorldPosition();
			level.Key('e');
			ReadDialogue(level);
			assert(journal.ConfluenceProgress() == Stage::Meeting);
			level.m_Player = level.m_StoryContact->WorldPosition();
			level.Key('e');
			assert(level.m_Dialogue.size() == 4);
			ReadDialogue(level);
			assert(journal.ConfluenceProgress() == Stage::Chart &&
			       Distance(level.StoryTarget(), level.m_SurveyStone->WorldPosition()) < .01f);
			level.m_Player = level.m_SurveyStone->WorldPosition();
			level.Key('e');
			ReadDialogue(level);
			assert(journal.ConfluenceProgress() == Stage::Complete &&
			       !journal.ReadConfluenceChart());
			assert(journal.NextDestination() != Region::Confluence);
			assert(TryExit(level, Region::Basin) && !TryExit(level, first) &&
			       TryExit(level, second));
			assert(!journal.CanTravel(first, Region::Confluence) &&
			       journal.CanTravel(second, Region::Confluence));

			MonitorSnapshot snapshot;
			snapshot.scene = WorldTravel::Key(Region::Confluence);
			level.CaptureMonitorState(snapshot);
			assert(snapshot.npcs.back().visible && snapshot.npcs.back().id == "Confluence/Isol");
			assert(snapshot.villages.back().rumors.size() == 3);

			const Region third = *std::find_if(std::begin(Contacts),
			                                   std::end(Contacts),
			                                   [first, second](Region region)
			                                   {
				                                   return region != first && region != second;
			                                   });
			assert(!journal.CanTravel(third, Region::Confluence));
			FinishRegionalStory(journal, third, 1);
			assert(TryExit(level, third) && journal.CanTravel(third, Region::Confluence));
			level.Begin(44, false, third, Region::Confluence);
			assert(TryExit(level, Region::Confluence));
			level.Begin(45, true, Region::Confluence, third);
			assert(journal.ConfluenceProgress() == Stage::Complete);
			level.m_Player = level.m_StoryContact->WorldPosition();
			level.Key('e');
			assert(level.m_Dialogue.size() == 4);
			ReadDialogue(level);

			journal.Reset();
			assert(journal.ConfluenceProgress() == Stage::Locked &&
			       journal.CompletedRegionalStories() == 0);
		}
}
