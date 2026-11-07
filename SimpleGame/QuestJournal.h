#pragma once
#include "WorldTravel.h"
#include <array>
#include <utility>

class QuestJournal
{
  public:
	enum class Stage
	{
		Elder,
		Scout,
		Rune,
		Fragment,
		Fisher,
		Shrine,
		Return,
		Complete
	};

	enum class DwarfStage
	{
		Meeting,
		Record,
		Report,
		Choice,
		Complete
	};

	enum class DragonStage
	{
		Meeting,
		Trace,
		Audience,
		Choice,
		Trial,
		Memory,
		Complete
	};

	QuestJournal();
	void Reset();
	bool CompleteStep(Stage expected);
	bool ChooseLake(int value);
	void Visit(WorldTravel::Region region);
	void RecordTravel(WorldTravel::Region from, WorldTravel::Region to);
	bool Investigate(WorldTravel::Region region);
	bool Visited(WorldTravel::Region region) const;
	bool Investigated(WorldTravel::Region region) const;
	bool StartDwarfStory();
	bool ReadDwarfRecord();
	bool HearDwarfCouncil();
	bool ChooseDwarf(int value);
	DwarfStage DwarfProgress() const;
	int DwarfChoice() const;
	bool StartDragonStory();
	bool ReadDragonTrace();
	bool HearDragon();
	bool ChooseDragon(int value);
	bool CompleteDragonTrial();
	bool ReadDragonMemory();
	DragonStage DragonProgress() const;
	int DragonApproach() const;
	int DragonChoice() const;
	bool CanTravel(WorldTravel::Region from, WorldTravel::Region to) const;
	const Stage& TutorialStage() const;
	const int& LakeChoice() const;
	const wchar_t* Objective() const;
	WorldTravel::Region NextDestination() const;
	std::vector<WorldTravel::Region> RouteToNext(WorldTravel::Region from) const;
	std::vector<std::wstring> Lines(int tab, WorldTravel::Region current) const;

	bool IsOpen() const;
	void Open();
	void HandleKey(unsigned char key);
	void Draw(Renderer& renderer, int width, int height, WorldTravel::Region current);

  private:
	static constexpr size_t RegionCount = static_cast<size_t>(WorldTravel::Region::Count);
	Stage m_Stage = Stage::Elder;
	int m_LakeChoice = 0;
	DwarfStage m_DwarfStage = DwarfStage::Meeting;
	int m_DwarfChoice = 0;
	DragonStage m_DragonStage = DragonStage::Meeting;
	int m_DragonApproach = 0;
	std::array<bool, RegionCount> m_Visited = {}, m_Investigated = {};
	std::vector<std::pair<WorldTravel::Region, WorldTravel::Region>> m_Routes;
	bool m_Open = false;
	int m_Tab = 0, m_Page = 0, m_PageCount = 1;
};
