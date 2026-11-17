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
	using Stage = QuestJournal::CoastStage;

	void FinishPages(FirstLevel& level)
	{
		while (!level.m_Dialogue.empty())
			level.Key('e');
	}
}

int main()
{
	for (int lake : {1, 2})
	{
		for (int choice : {1, 2})
		{
			QuestJournal journal;
			FirstLevel level(journal);
			level.Begin(42, true, Region::MistShore, Region::EchoHall);
			level.m_Player = level.m_StoryContact->WorldPosition();
			level.Key('e');
			FinishPages(level);
			assert(journal.CoastProgress() == Stage::Dream);
			assert(!journal.CanTravel(Region::MistShore, Region::TidalRuins));
			for (auto stage : {QuestJournal::Stage::Elder,
			                   QuestJournal::Stage::Scout,
			                   QuestJournal::Stage::Rune,
			                   QuestJournal::Stage::Fragment,
			                   QuestJournal::Stage::Fisher})
				assert(journal.CompleteStep(stage));
			assert(journal.ChooseLake(lake));
			assert(journal.CompleteStep(QuestJournal::Stage::Return));
			level.Key('e');
			assert(journal.CoastProgress() == Stage::Dream);
			assert(level.m_Dialogue[1].find(lake == 1 ? L"울린 종소리" : L"종을 감싸자") !=
			       std::wstring::npos);
			FinishPages(level);
			assert(journal.NextDestination() == Region::TidalRuins);
			assert(!journal.ReadTidalShrine());
			level.m_Player = WorldTravel::FarGate;
			assert(level.Key('e') && level.RequestedExit() == Region::TidalRuins);
			level.Begin(43, false, Region::TidalRuins, Region::MistShore);
			level.m_Player = level.m_StoryContact->WorldPosition();
			level.m_Hp = 37;
			level.Key('e');
			FinishPages(level);
			assert(level.IsStoryOpen() && !level.CanOpenJournal());
			level.Key('3');
			assert(journal.CoastProgress() == Stage::Choice);
			level.Key(static_cast<unsigned char>('0' + choice));
			FinishPages(level);
			assert(journal.CoastChoice() == choice && journal.LakeChoice() == lake);
			assert(journal.CoastProgress() == Stage::Complete);
			assert(!journal.ChooseCoast(3 - choice) && level.m_Experience == 75 &&
			       level.m_Hp == 37);

			MonitorSnapshot snapshot;
			level.CaptureMonitorState(snapshot);
			assert(snapshot.mobs[10].total == 24 && snapshot.mobs[11].total == 24);
			assert(snapshot.villages.back().disposition ==
			       (choice == 1 ? "조심스러운 접촉" : "연결 중단"));
			level.m_Player = WorldTravel::NearGate;
			assert(level.Key('e') && level.RequestedExit() == Region::MistShore);
			level.Begin(44, true, Region::MistShore, Region::TidalRuins);
			assert(journal.CoastChoice() == choice && journal.LakeChoice() == lake);
			assert(!level.IsStoryOpen() && level.m_Experience == 0);
			journal.Reset();
			assert(journal.CoastProgress() == Stage::Dream && journal.CoastChoice() == 0);
		}
	}
}
