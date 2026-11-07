#pragma once
#include "Renderer.h"

namespace WorldTravel
{
	constexpr Point VillageGate = {97, 99};
	constexpr Point VillageLanding = {95.5f, 99};
	constexpr int Columns = 48, Rows = 40;
	constexpr int VisualMargin = 96;
	constexpr Point NearGate = {23, 23};
	constexpr Point FarGate = {73, 57};
	constexpr Point SurveyStone = {49, 41};

	enum class Region
	{
		None,
		Village,
		Passage,
		Basin,
		EchoHall,
		StoneRidge,
		MistShore,
		DwarfMine,
		DwarfCity,
		Highlands,
		DragonRoost,
		Count
	};

	struct ExitLink
	{
		Point position;
		Region destination;
	};

	inline const std::vector<ExitLink>& Exits(Region region)
	{
		static const std::vector<ExitLink> links[] = {
		    {},
		    {{VillageGate, Region::Passage}},
		    {{NearGate, Region::Village}, {FarGate, Region::Basin}},
		    {{NearGate, Region::Passage},
		     {FarGate, Region::EchoHall},
		     {{73, 23}, Region::DwarfMine},
		     {{49, 57}, Region::Highlands}},
		    {{NearGate, Region::Basin},
		     {{73, 41}, Region::StoneRidge},
		     {{49, 57}, Region::MistShore}},
		    {{NearGate, Region::EchoHall},
		     {FarGate, Region::DwarfMine},
		     {{73, 23}, Region::DragonRoost}},
		    {{NearGate, Region::EchoHall}},
		    {{NearGate, Region::StoneRidge},
		     {FarGate, Region::DwarfCity},
		     {{73, 23}, Region::Basin}},
		    {{NearGate, Region::DwarfMine}},
		    {{NearGate, Region::Basin}, {FarGate, Region::DragonRoost}},
		    {{NearGate, Region::Highlands}, {FarGate, Region::StoneRidge}}};
		return links[static_cast<int>(region)];
	}

	inline bool IsPassage(Region region)
	{
		return region == Region::Passage || region == Region::EchoHall;
	}

	inline const wchar_t* Name(Region region)
	{
		static const wchar_t* names[] = {L"",
		                                 L"갈대여울",
		                                 L"옛 드워프 교역로",
		                                 L"솔바람 분지",
		                                 L"메아리 숲길",
		                                 L"돌등성이 전초지",
		                                 L"안개 호숫가",
		                                 L"깊은돌 폐광길",
		                                 L"깊은돌 도시 앞마당",
		                                 L"바람등 고원",
		                                 L"잿빛 날개 쉼터"};
		return names[static_cast<int>(region)];
	}

	inline const char* Key(Region region)
	{
		static const char* keys[] = {"",
		                             "tutorial",
		                             "first_level",
		                             "basin",
		                             "echo_hall",
		                             "stone_ridge",
		                             "mist_shore",
		                             "dwarf_mine",
		                             "dwarf_city",
		                             "highlands",
		                             "dragon_roost"};
		return keys[static_cast<int>(region)];
	}

	inline bool OuterEdge(Point p, float width, float height, float margin)
	{
		return p.x < margin || p.y < margin || p.x > width - margin || p.y > height - margin;
	}

	struct ReturnTrail
	{
		Point lastSafe = {18, 50};

		bool ReturnIfLost(Point& position, float width, float height)
		{
			if (OuterEdge(position, width, height, 4))
			{
				position = lastSafe;
				return true;
			}
			if (!OuterEdge(position, width, height, 12))
				lastSafe = position;
			return false;
		}
	};

	inline void DrawGate(Renderer& r, Point p, bool daylight)
	{
		r.SoftEllipse(p.x, p.y + 4, 48, 16, {.02f, .03f, .03f, .5f});
		r.Polygon({{p.x - 43, p.y},
		           {p.x - 42, p.y - 61},
		           {p.x - 19, p.y - 88},
		           {p.x + 24, p.y - 88},
		           {p.x + 47, p.y - 59},
		           {p.x + 47, p.y}},
		          {.35f, .38f, .34f, 1},
		          Material::Stone);
		r.Polygon({{p.x - 24, p.y},
		           {p.x - 24, p.y - 53},
		           {p.x - 12, p.y - 68},
		           {p.x + 14, p.y - 68},
		           {p.x + 28, p.y - 52},
		           {p.x + 28, p.y}},
		          daylight ? Color{.65f, .78f, .49f, 1} : Color{.025f, .04f, .045f, 1});
		r.Rect(p.x - 5, p.y - 83, 10, 10, {.60f, 1.05f, .95f, 1});
		r.Rect(p.x - 59, p.y - 31, 4, 34, {.34f, .24f, .14f, 1});
		r.Polygon({{p.x - 65, p.y - 54},
		           {p.x - 48, p.y - 54},
		           {p.x - 50, p.y - 25},
		           {p.x - 63, p.y - 25}},
		          {.98f, .48f, .12f, .95f},
		          Material::Flame,
		          {{-1, 1}, {1, 1}, {1, 0}, {-1, 0}});
	}
}
