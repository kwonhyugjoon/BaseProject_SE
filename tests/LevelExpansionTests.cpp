#include "../SimpleGame/FirstLevel.h"
#include "../SimpleGame/QuestJournal.h"
#include "../SimpleGame/MonitorState.h"
#include <cassert>
#include <cmath>
#include <set>
#include <cstdio>

int main()
{
	QuestJournal journal;
	FirstLevel level(journal);
	MonitorSnapshot snapshot;
	level.CaptureMonitorState(snapshot);
	assert(!snapshot.huntingLoaded && snapshot.mobs[0].total == 0);
	for (unsigned seed : {1u, 42u, 930u})
	{
		level.Begin(seed, true);
		Actor* enemies = level.Scene().World().Find("Enemies");
		assert(enemies && enemies->Children().size() == 48);
		int sectors[16] = {};
		std::set<std::pair<int, int>> positions;
		std::set<std::string> types;
		for (const auto& actor : enemies->Children())
		{
			Point p = actor->WorldPosition();
			assert(p.x > 0 && p.x < 96 && p.y > 0 && p.y < 80);
			assert(std::hypot(p.x - WorldTravel::NearGate.x, p.y - WorldTravel::NearGate.y) > 12);
			assert(positions.insert({static_cast<int>(p.x), static_cast<int>(p.y)}).second);
			++sectors[static_cast<int>(p.y) / 20 * 4 + static_cast<int>(p.x) / 24];
			types.insert(actor->Name().substr(0, actor->Name().find('/')));
		}
		for (int count : sectors)
			assert(count == 3);
		assert(types.size() == 3 && types.count("Wolf") && !types.count("Priest") &&
		       !types.count("Slime"));
		level.CaptureMonitorState(snapshot);
		int monitoredTotal = 0, monitoredAlive = 0;
		for (const auto& count : snapshot.mobs)
		{
			assert(count.alive == count.total);
			monitoredTotal += count.total;
			monitoredAlive += count.alive;
		}
		assert(snapshot.huntingLoaded && monitoredTotal == 48 && monitoredAlive == 48);
		printf("Seed %u: 48 unique spawns, 16 populated sectors, three trade-road enemy types.\n",
		       seed);
	}
	Actor* enemies = level.Scene().World().Find("Enemies");
	std::set<std::string> selected;
	std::vector<Actor*> targets;
	for (const auto& actor : enemies->Children())
	{
		std::string type = actor->Name().substr(0, actor->Name().find('/'));
		if (selected.insert(type).second)
		{
			actor->SetWorldPosition({WorldTravel::NearGate.x + .8f, WorldTravel::NearGate.y + .8f});
			targets.push_back(actor.get());
		}
	}
	bool keys[256] = {};
	keys[' '] = true;
	for (int i = 0; i < 100; ++i)
		level.Update(1.f / 60, keys);
	for (Actor* actor : targets)
		assert(!actor->IsVisible());
	level.CaptureMonitorState(snapshot);
	int dead = 0;
	for (const auto& count : snapshot.mobs)
		dead += count.total - count.alive;
	assert(dead == 3);
	Actor* drops = level.Scene().World().Find("Drops");
	assert(drops->Children().size() == 3);
	level.Key('e');
	assert(drops->Children().empty());
	keys[' '] = false;
	for (int i = 0; i < 60; ++i)
		level.Update(.25f, keys);
	for (Actor* actor : targets)
		assert(actor->IsVisible());
	level.CaptureMonitorState(snapshot);
	for (const auto& count : snapshot.mobs)
		assert(count.alive == count.total);
	puts("Trade-road kills, themed loot pickup and respawn passed without an OpenGL window.");
}
