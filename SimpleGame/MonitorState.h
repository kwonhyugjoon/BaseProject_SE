#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct MonitorNpc
{
	std::string id, name, villageId, routine, state;
	float x = 0, y = 0;
	int facing = 0;
	bool visible = false, enabled = false;
};

struct MonitorVillage
{
	std::string id, name;
	int population = 0;
	std::string mood, disposition;
	bool rumorsImplemented = false;
	std::vector<std::string> rumors;
	std::string lakeChoice;
};

struct MonitorMobCount
{
	std::string kind, name;
	int total = 0, alive = 0;
};

struct MonitorSnapshot
{
	uint64_t sampledAt = 0;
	std::string scene;
	bool paused = false, huntingLoaded = false;
	std::vector<MonitorNpc> npcs;
	std::vector<MonitorVillage> villages;
	std::array<MonitorMobCount, 10> mobs;
};

std::string MonitorUtf8(const wchar_t* text);
std::string SerializeMonitorSnapshot(const MonitorSnapshot& snapshot);
