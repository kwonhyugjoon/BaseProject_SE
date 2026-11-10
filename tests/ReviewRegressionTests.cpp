#include "../SimpleGame/stdafx.h"
#include "../SimpleGame/SceneGraph.h"
#include "../SimpleGame/LevelModels.h"
#include "../SimpleGame/WorldTravel.h"
#include "../SimpleGame/QuestJournal.h"
#include "../SimpleGame/MonitorState.h"
#include <array>
#include <cmath>
#define private public
#include "../SimpleGame/FirstLevel.h"
#undef private
#define main GameApplicationMain
#include "../SimpleGame/SimpleGame.cpp"
#undef main
#include <cstdio>
#include <cassert>

size_t CountNodes(const Actor& node)
{
	size_t count = 1;
	for (const auto& child : node.Children())
		count += CountNodes(*child);
	return count;
}

int main()
{
	BuildTutorialForest();
	BuildTutorialScene();
	Reset();
	dialogue.clear();
	printf("VILLAGE_COUNTS trees=%zu graphNodes=%zu\n",
	       trees.size(),
	       CountNodes(tutorialScene.Root()));
	EnterRegion(WorldTravel::Region::Passage);
	firstLevel.m_Player = WorldTravel::NearGate;
	for (auto& enemy : firstLevel.m_Enemies)
	{
		enemy.position = {80, 70};
		enemy.hp = 1;
		enemy.cooldown = 100;
	}
	auto& target = firstLevel.m_Enemies[0];
	target.position = {23.8f, 23.8f};
	target.hp = 1;
	WindowKey(nullptr, GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
	WindowKey(nullptr, GLFW_KEY_SPACE, 0, GLFW_RELEASE, 0);
	firstLevel.Update(1.f / 60, keys);
	printf("TAP_ATTACK hp=%d swing=%.3f held=%d\n", target.hp, firstLevel.m_Swing, keys[' ']);
	assert(target.hp == 0 && firstLevel.m_Swing > 0);
	target.hp = 1;
	firstLevel.m_AttackCooldown = 0;
	WindowKey(nullptr, GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
	firstLevel.Update(1.f / 60, keys);
	printf("HELD_ATTACK hp=%d swing=%.3f\n", target.hp, firstLevel.m_Swing);
	assert(target.hp == 0 && firstLevel.m_Swing > 0);
	WindowKey(nullptr, GLFW_KEY_SPACE, 0, GLFW_RELEASE, 0);
	firstLevel.Key('m');
	auto snap = CaptureMonitorState();
	auto beforeTime = firstLevel.m_Time;
	firstLevel.Update(.1f, keys);
	printf("MAP_MONITOR paused=%d worldTimeAdvanced=%d\n",
	       snap.paused,
	       firstLevel.m_Time > beforeTime);
	assert(snap.paused && firstLevel.m_Time == beforeTime);
	firstLevel.Key('m');
	printf("HUNT_COUNTS graphNodes=%zu enemyNodes=%zu\n",
	       CountNodes(firstLevel.Scene().Root()),
	       CountNodes(*firstLevel.Scene().World().Find("Enemies")));
	firstLevel.m_Tiles.fill(FirstLevel::Tile::Ground);
	std::fill(std::begin(keys), std::end(keys), false);
	firstLevel.m_Player = {41, 27};
	for (auto& enemy : firstLevel.m_Enemies)
	{
		enemy.hp = 0;
		enemy.respawn = 100;
	}
	auto& walker = firstLevel.m_Enemies[0];
	walker.hp = 36;
	walker.kind = FirstLevel::EnemyKind::Bandit;
	walker.position = {35, 27};
	walker.windup = 0;
	walker.charge = 0;
	walker.cooldown = 100;
	Point walkStart = walker.position;
	firstLevel.Update(1.f / 60 + 0.000001f, keys);
	printf("MOVING_ANIMATION distance=%.6f moving=%d\n",
	       Distance(walkStart, walker.position),
	       walker.moving);
	assert(Distance(walkStart, walker.position) > .02f && walker.moving);
	firstLevel.m_Level = 4;
	firstLevel.m_Experience = 180;
	firstLevel.m_Hp = 37;
	firstLevel.m_Materials[3] = 7;
	inFirstLevel = false;
	paused = true;
	KeyDown('r', 0, 0);
	dialogue.clear();
	EnterRegion(WorldTravel::Region::Passage);
	printf("NEW_TUTORIAL oldLevel=%d oldXP=%d oldHP=%d oldMaterial=%d tutorialStage=%d\n",
	       firstLevel.m_Level,
	       firstLevel.m_Experience,
	       firstLevel.m_Hp,
	       firstLevel.m_Materials[3],
	       static_cast<int>(stage));
	assert(stage == Elder && firstLevel.m_Level == 1 && firstLevel.m_Experience == 0 &&
	       firstLevel.m_Hp == 100 && firstLevel.m_Materials[3] == 0);
	// An attack queued during cooldown must be cancelled by a modal or focus loss.
	firstLevel.m_AttackCooldown = .2f;
	firstLevel.Key(' ');
	firstLevel.Key('m');
	firstLevel.Key('m');
	assert(!firstLevel.m_AttackQueued);
	firstLevel.Key(' ');
	FocusChanged(nullptr, GLFW_FALSE);
	assert(!firstLevel.m_AttackQueued);
	for (unsigned char modal : {'m', 'i'})
	{
		firstLevel.Key(modal);
		assert(CaptureMonitorState().paused);
		firstLevel.Key(modal);
		assert(!CaptureMonitorState().paused);
	}
	firstLevel.m_Hp = 0;
	assert(CaptureMonitorState().paused);
	firstLevel.Key('r');
	assert(!CaptureMonitorState().paused);
	inFirstLevel = false;
	dialogue.clear();
	Point oldTree = trees[0].actor.WorldPosition();
	trees[0].actor.SetWorldPosition({3, 3});
	RebuildTreeSpatialData();
	assert(treeCollision.Blocks({3, 3}));
	Point& borrowedTree = trees[0].p;
	borrowedTree = {8, 8};
	RebuildTreeSpatialData();
	assert(!treeCollision.Blocks({3, 3}) && treeCollision.Blocks({8, 8}));
	trees[0].actor.SetWorldPosition(oldTree);
	RebuildTreeSpatialData();
	treeActors.SetLocalPosition({1, 0});
	RebuildTreeSpatialData();
	assert(treeSpatialPositions[0].x == oldTree.x + 1 && treeSpatialPositions[0].y == oldTree.y);
	treeActors.SetLocalPosition({0, 0});
	RebuildTreeSpatialData();
	puts(
	    "PASS: tap/held attack, animation remainder, new game, modal/focus cancellation, monitor pause and moved-tree collisions.");
}
