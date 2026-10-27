// Exercise region routing through the real input handlers without a game window.
#define main TutorialApplicationMain
#include "../SimpleGame/SimpleGame.cpp"
#undef main
#include <cassert>
#include <queue>
#include <cstdio>

int main()
{
	BuildTutorialForest();
	BuildTutorialScene();
	Reset();
	dialogue.clear();
	assert(dungeonGate && CanWalk(WorldTravel::VillageLanding));
	assert(CanWalk(WorldTravel::VillageGate));
	// All tutorial landmarks must remain connected after filling the forest.
	std::vector<bool> reached(MapWidth * MapHeight);
	std::queue<int> pending;
	int start = 50 * MapWidth + 18;
	reached[start] = true;
	pending.push(start);
	while (!pending.empty())
	{
		int cell = pending.front();
		pending.pop();
		for (int next : {cell - 1, cell + 1, cell - MapWidth, cell + MapWidth})
		{
			if (next < 0 || next >= MapWidth * MapHeight || reached[next] ||
			    std::abs(next % MapWidth - cell % MapWidth) +
			            std::abs(next / MapWidth - cell / MapWidth) !=
			        1)
				continue;
			if (!CanWalk(
			        {static_cast<float>(next % MapWidth), static_cast<float>(next / MapWidth)}))
				continue;
			reached[next] = true;
			pending.push(next);
		}
	}
	for (Point target : {Point{18, 50},
	                     Point{12, 29},
	                     rune,
	                     fragment,
	                     shrine,
	                     Point{45, 43},
	                     WorldTravel::VillageGate})
		assert(reached[static_cast<int>(target.y) * MapWidth + static_cast<int>(target.x)]);
	KeyDown('l', 0, 0);
	assert(!inFirstLevel);
	KeyDown('e', 0, 0);
	assert(!inFirstLevel);
	dialogue.clear();
	stage = Shrine;
	choice = 2;
	playerActor.SetWorldPosition(WorldTravel::VillageLanding);
	assert(Nearby() == PersonCount + 3);
	KeyDown('e', 0, 0);
	assert(inFirstLevel && firstLevel.CurrentRegion() == FirstLevel::Region::Passage);
	assert(firstLevel.Scene().World().Find("Exit/1"));
	assert(!keys['e']);
	Actor* expeditionPlayer = firstLevel.Scene().World().Find("Player");
	expeditionPlayer->SetWorldPosition(WorldTravel::FarGate);
	KeyDown('e', 0, 0);
	assert(firstLevel.CurrentRegion() == FirstLevel::Region::Basin && inFirstLevel);
	assert(CaptureMonitorState().scene == "basin");
	assert(firstLevel.Scene().World().Find("SurveyStone"));
	// The second dungeon has three exits with distinct, reciprocal destinations.
	expeditionPlayer->SetWorldPosition(WorldTravel::FarGate);
	KeyDown('e', 0, 0);
	assert(firstLevel.CurrentRegion() == FirstLevel::Region::EchoHall);
	for (int branch : {1, 2})
	{
		const auto& link = WorldTravel::Exits(FirstLevel::Region::EchoHall)[branch];
		expeditionPlayer->SetWorldPosition(link.position);
		KeyDown('e', 0, 0);
		assert(firstLevel.CurrentRegion() == link.destination);
		assert(CaptureMonitorState().scene == WorldTravel::Key(link.destination));
		KeyDown('e', 0, 0);
		assert(firstLevel.CurrentRegion() == FirstLevel::Region::EchoHall);
		assert(Distance(expeditionPlayer->WorldPosition(), link.position) < .001f);
	}
	expeditionPlayer->SetWorldPosition(WorldTravel::NearGate);
	KeyDown('e', 0, 0);
	assert(firstLevel.CurrentRegion() == FirstLevel::Region::Basin);
	assert(Distance(expeditionPlayer->WorldPosition(), WorldTravel::FarGate) < .001f);
	expeditionPlayer->SetWorldPosition(WorldTravel::NearGate);
	KeyDown('m', 0, 0);
	KeyDown('e', 0, 0);
	assert(firstLevel.CurrentRegion() == FirstLevel::Region::Basin);
	KeyDown('m', 0, 0);
	KeyDown('e', 0, 0);
	assert(firstLevel.CurrentRegion() == FirstLevel::Region::Passage);
	Point returned = expeditionPlayer->WorldPosition();
	assert(Distance(returned, WorldTravel::FarGate) < .001f);
	expeditionPlayer->SetWorldPosition(WorldTravel::NearGate);
	KeyDown('e', 0, 0);
	assert(!inFirstLevel && stage == Shrine && choice == 2);
	assert(Distance(player, WorldTravel::VillageLanding) < .001f);
	assert(Distance(camera, player) < .001f && !keys['e']);
	// Pause/map/dialogue block entrance input.
	showMap = true;
	KeyDown('e', 0, 0);
	assert(!inFirstLevel);
	showMap = false;
	paused = true;
	KeyDown('e', 0, 0);
	assert(!inFirstLevel);
	paused = false;
	// Forest return restores a walkable position and clears held movement.
	forestTrail.lastSafe = WorldTravel::VillageLanding;
	playerActor.SetWorldPosition({3, 50});
	keys['a'] = true;
	UpdatePlayer(1.f / 120);
	assert(Distance(player, WorldTravel::VillageLanding) < .001f);
	assert(CanWalk(player) && !keys['a'] && travelNoticeTime > 0);
	assert(stage == Shrine && choice == 2);
	puts("PASS: tutorial routes, E-only round trip, overlays, history, input and forest return.");
}
