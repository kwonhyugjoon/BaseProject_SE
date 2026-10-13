#include "../SimpleGame/FirstLevel.h"
#include "../SimpleGame/MonitorState.h"
#include <cassert>
#include <cmath>
#include <set>
#include <cstdio>

int main()
{
	FirstLevel level;
	MonitorSnapshot snapshot;
	level.CaptureMonitorState(snapshot);
	assert(!snapshot.huntingLoaded && snapshot.mobs[0].total == 0);
	for (unsigned seed : {1u, 42u, 930u})
	{
		level.Begin(seed, true);
		Actor* enemies = level.Scene().World().Find("Enemies");
		assert(enemies && enemies->Children().size() == 192);
		int sectors[64] = {};
		std::set<std::pair<int, int>> positions;
		std::set<std::string> types;
		for (const auto& actor : enemies->Children())
		{
			Point p = actor->WorldPosition();
			assert(p.x > 0 && p.x < 192 && p.y > 0 && p.y < 160);
			assert(std::hypot(p.x - 7, p.y - 7) > 12);
			assert(positions.insert({static_cast<int>(p.x), static_cast<int>(p.y)}).second);
			++sectors[static_cast<int>(p.y) / 20 * 8 + static_cast<int>(p.x) / 24];
			types.insert(actor->Name().substr(0, actor->Name().find('/')));
		}
		for (int count : sectors)
			assert(count == 3);
		assert(types.size() == 4);
		level.CaptureMonitorState(snapshot);
		int monitoredTotal = 0, monitoredAlive = 0;
		for (const auto& count : snapshot.mobs)
		{
			assert(count.total > 0 && count.alive == count.total);
			monitoredTotal += count.total;
			monitoredAlive += count.alive;
		}
		assert(snapshot.huntingLoaded && monitoredTotal == 192 && monitoredAlive == 192);
		printf("Seed %u: 192 unique spawns, 64 populated sectors, four enemy types.\n", seed);
	}
	Actor* enemies = level.Scene().World().Find("Enemies");
	std::set<std::string> selected;
	std::vector<Actor*> targets;
	for (const auto& actor : enemies->Children())
	{
		std::string type = actor->Name().substr(0, actor->Name().find('/'));
		if (selected.insert(type).second)
		{
			actor->SetWorldPosition({7.8f, 7.8f});
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
	assert(dead == 4);
	Actor* drops = level.Scene().World().Find("Drops");
	assert(drops->Children().size() == 4);
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
	puts("Four-type kills, loot pickup and respawn passed without an OpenGL window.");
}
