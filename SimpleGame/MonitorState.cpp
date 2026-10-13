#include "stdafx.h"
#include "MonitorState.h"
#include <Windows.h>
#include <iomanip>
#include <locale>
#include <sstream>

namespace
{
	void String(std::ostream& out, const std::string& text)
	{
		const char* hex = "0123456789abcdef";
		out << '"';
		for (unsigned char ch : text)
		{
			if (ch == '"' || ch == '\\')
				out << '\\' << ch;
			else if (ch < 0x20)
				out << "\\u00" << hex[ch >> 4] << hex[ch & 15];
			else
				out << ch;
		}
		out << '"';
	}

	void OptionalString(std::ostream& out, const std::string& text)
	{
		if (text.empty())
			out << "null";
		else
			String(out, text);
	}
}

std::string MonitorUtf8(const wchar_t* text)
{
	int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
	if (size <= 1)
		return {};
	std::string result(size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text, -1, &result[0], size, nullptr, nullptr);
	result.pop_back();
	return result;
}

std::string SerializeMonitorSnapshot(const MonitorSnapshot& snapshot)
{
	int total = 0, alive = 0;
	for (const auto& mob : snapshot.mobs)
	{
		total += mob.total;
		alive += mob.alive;
	}

	std::ostringstream out;
	out.imbue(std::locale::classic());
	out << std::boolalpha << std::fixed << std::setprecision(2);
	out << "{\"schemaVersion\":1,\"sampledAt\":" << snapshot.sampledAt << ",\"scene\":";
	String(out, snapshot.scene);
	out << ",\"paused\":" << snapshot.paused << ",\"counts\":{\"npcs\":" << snapshot.npcs.size()
	    << ",\"villages\":" << snapshot.villages.size() << ",\"mobs\":" << total
	    << ",\"aliveMobs\":" << alive << ",\"deadMobs\":" << total - alive << "},\"villages\":[";

	for (size_t i = 0; i < snapshot.villages.size(); ++i)
	{
		const auto& village = snapshot.villages[i];
		if (i)
			out << ',';
		out << "{\"id\":";
		String(out, village.id);
		out << ",\"name\":";
		String(out, village.name);
		out << ",\"population\":" << village.population << ",\"mood\":";
		OptionalString(out, village.mood);
		out << ",\"disposition\":";
		OptionalString(out, village.disposition);
		out << ",\"lakeChoice\":";
		OptionalString(out, village.lakeChoice);
		out << ",\"rumors\":";
		if (!village.rumorsImplemented)
			out << "null";
		else
		{
			out << '[';
			for (size_t rumor = 0; rumor < village.rumors.size(); ++rumor)
			{
				if (rumor)
					out << ',';
				String(out, village.rumors[rumor]);
			}
			out << ']';
		}
		out << '}';
	}

	out << "],\"npcs\":[";
	for (size_t i = 0; i < snapshot.npcs.size(); ++i)
	{
		const auto& npc = snapshot.npcs[i];
		if (i)
			out << ',';
		out << "{\"id\":";
		String(out, npc.id);
		out << ",\"name\":";
		String(out, npc.name);
		out << ",\"villageId\":";
		String(out, npc.villageId);
		out << ",\"routine\":";
		String(out, npc.routine);
		out << ",\"state\":";
		String(out, npc.state);
		out << ",\"position\":{\"x\":" << npc.x << ",\"y\":" << npc.y
		    << "},\"facing\":" << npc.facing << ",\"visible\":" << npc.visible
		    << ",\"enabled\":" << npc.enabled << '}';
	}

	out << "],\"hunting\":{\"loaded\":" << snapshot.huntingLoaded << ",\"types\":[";
	for (size_t i = 0; i < snapshot.mobs.size(); ++i)
	{
		const auto& mob = snapshot.mobs[i];
		if (i)
			out << ',';
		out << "{\"kind\":";
		String(out, mob.kind);
		out << ",\"name\":";
		String(out, mob.name);
		out << ",\"total\":" << mob.total << ",\"alive\":" << mob.alive
		    << ",\"dead\":" << mob.total - mob.alive << '}';
	}
	out << "]}}";
	return out.str();
}
