#include "../SimpleGame/MonitorServer.h"
#include "../SimpleGame/MonitorState.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <locale>

namespace
{
	class CommaDecimal : public std::numpunct<char>
	{
		char do_decimal_point() const override
		{
			return ',';
		}
	};
}

int main()
{
	std::locale::global(std::locale(std::locale::classic(), new CommaDecimal));
	MonitorSnapshot snapshot;
	snapshot.scene = "tutorial";
	snapshot.huntingLoaded = true;
	MonitorVillage village;
	village.id = "reedmere";
	village.name = "갈대여울과 주변";
	village.population = 2;
	snapshot.villages.push_back(village);
	MonitorNpc npc;
	npc.id = "People/0";
	npc.name = MonitorUtf8(L"주민 \"가\"\\길\n\t🌿");
	npc.villageId = village.id;
	npc.routine = "퀘스트 고정 배치";
	npc.state = "standing";
	npc.x = 12.25f;
	npc.y = 3.5f;
	npc.visible = npc.enabled = true;
	snapshot.npcs.push_back(npc);
	npc.id = "People/1";
	npc.name = "정찰병 라온";
	npc.routine = "주변 순찰";
	npc.state = "walking";
	snapshot.npcs.push_back(npc);
	snapshot.mobs[0] = {"slime", "슬라임", 1, 1};
	snapshot.mobs[1] = {"bandit", "도적", 1, 0};
	snapshot.mobs[2] = {"archer", "궁수", 1, 1};
	snapshot.mobs[3] = {"priest", "타락한 사제", 1, 1};
	MonitorServer server;
	if (!server.Start())
		return 2;
	MonitorServer duplicate;
	assert(!duplicate.Start());
	auto publish = [&]()
	{
		snapshot.sampledAt = std::chrono::duration_cast<std::chrono::milliseconds>(
		                         std::chrono::system_clock::now().time_since_epoch())
		                         .count();
		server.Publish(SerializeMonitorSnapshot(snapshot));
	};
	publish();
	std::cout << "READY\n" << std::flush;
	std::string command;
	while (std::getline(std::cin, command) && command != "quit")
	{
		if (command == "pause")
		{
			snapshot.paused = true;
			for (auto& person : snapshot.npcs)
				person.state = "paused";
		}
		if (command == "clear")
			server.Publish("");
		else
			publish();
		std::cout << "UPDATED\n" << std::flush;
	}
	server.Stop();
}
