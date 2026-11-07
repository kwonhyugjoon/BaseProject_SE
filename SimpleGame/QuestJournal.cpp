#include "stdafx.h"
#include "QuestJournal.h"
#include <algorithm>
#include <iterator>

namespace
{
	using Region = WorldTravel::Region;
	const Region SurveyRegions[] = {Region::Basin, Region::StoneRidge, Region::MistShore};
	const wchar_t* TutorialObjectives[] = {L"광장에서 촌장 엘린과 대화하기",
	                                       L"숲길을 따라 숲지기 로안 만나기",
	                                       L"숲 안쪽의 오래된 표석 조사하기",
	                                       L"동쪽 공터에서 지도 조각 조사하기",
	                                       L"호수 남쪽의 어부 미라 만나기",
	                                       L"호수 서쪽의 돌 제단 조사하기",
	                                       L"마을로 돌아가 엘린에게 이야기하기"};
}

QuestJournal::QuestJournal()
{
	Reset();
}

void QuestJournal::Reset()
{
	m_Stage = Stage::Elder;
	m_LakeChoice = 0;
	m_DwarfStage = DwarfStage::Meeting;
	m_DwarfChoice = 0;
	m_DragonStage = DragonStage::Meeting;
	m_DragonApproach = 0;
	m_Visited.fill(false);
	m_Investigated.fill(false);
	m_Routes.clear();
	m_Open = false;
	m_Tab = m_Page = 0;
	m_PageCount = 1;
	Visit(Region::Village);
}

bool QuestJournal::CompleteStep(Stage expected)
{
	if (m_Stage != expected || expected == Stage::Shrine || expected == Stage::Complete)
		return false;
	m_Stage = static_cast<Stage>(static_cast<int>(m_Stage) + 1);
	return true;
}

bool QuestJournal::ChooseLake(int value)
{
	if (m_Stage != Stage::Shrine || m_LakeChoice != 0 || (value != 1 && value != 2))
		return false;
	m_LakeChoice = value;
	m_Stage = Stage::Return;
	return true;
}

void QuestJournal::Visit(Region region)
{
	m_Visited[static_cast<size_t>(region)] = true;
}

bool QuestJournal::Visited(Region region) const
{
	return m_Visited[static_cast<size_t>(region)];
}

bool QuestJournal::Investigated(Region region) const
{
	return m_Investigated[static_cast<size_t>(region)];
}

void QuestJournal::RecordTravel(Region from, Region to)
{
	if (!CanTravel(from, to))
		return;
	const auto& exits = WorldTravel::Exits(from);
	if (std::none_of(exits.begin(),
	                 exits.end(),
	                 [to](const WorldTravel::ExitLink& exit)
	                 {
		                 return exit.destination == to;
	                 }))
		return;
	Visit(from);
	Visit(to);
	for (const auto& route : m_Routes)
		if ((route.first == from && route.second == to) ||
		    (route.first == to && route.second == from))
			return;
	m_Routes.push_back({from, to});
}

bool QuestJournal::Investigate(Region region)
{
	if (std::find(std::begin(SurveyRegions), std::end(SurveyRegions), region) ==
	        std::end(SurveyRegions) ||
	    !Visited(region) || Investigated(region))
		return false;
	m_Investigated[static_cast<size_t>(region)] = true;
	return true;
}

bool QuestJournal::StartDwarfStory()
{
	if (m_Stage != Stage::Complete || !Investigated(Region::StoneRidge) ||
	    m_DwarfStage != DwarfStage::Meeting)
		return false;
	m_DwarfStage = DwarfStage::Record;
	return true;
}

bool QuestJournal::ReadDwarfRecord()
{
	if (m_DwarfStage != DwarfStage::Record || !Visited(Region::DwarfMine))
		return false;
	m_DwarfStage = DwarfStage::Report;
	return true;
}

bool QuestJournal::HearDwarfCouncil()
{
	if (m_DwarfStage != DwarfStage::Report || !Visited(Region::DwarfCity))
		return false;
	m_DwarfStage = DwarfStage::Choice;
	return true;
}

bool QuestJournal::ChooseDwarf(int value)
{
	if (m_DwarfStage != DwarfStage::Choice || (value != 1 && value != 2))
		return false;
	m_DwarfChoice = value;
	m_DwarfStage = DwarfStage::Complete;
	return true;
}

QuestJournal::DwarfStage QuestJournal::DwarfProgress() const
{
	return m_DwarfStage;
}

int QuestJournal::DwarfChoice() const
{
	return m_DwarfChoice;
}

bool QuestJournal::StartDragonStory()
{
	if (m_Stage != Stage::Complete || !Visited(Region::Highlands) ||
	    m_DragonStage != DragonStage::Meeting)
		return false;
	m_DragonStage = DragonStage::Trace;
	return true;
}

bool QuestJournal::ReadDragonTrace()
{
	if (m_DragonStage != DragonStage::Trace || !Visited(Region::Highlands))
		return false;
	m_DragonStage = DragonStage::Audience;
	return true;
}

bool QuestJournal::HearDragon()
{
	if (m_DragonStage != DragonStage::Audience || !Visited(Region::DragonRoost))
		return false;
	m_DragonStage = DragonStage::Choice;
	return true;
}

bool QuestJournal::ChooseDragon(int value)
{
	if (m_DragonStage != DragonStage::Choice || value < 1 || value > 3)
		return false;
	m_DragonApproach = value;
	m_DragonStage = value == 2 ? DragonStage::Trial : DragonStage::Memory;
	return true;
}

bool QuestJournal::CompleteDragonTrial()
{
	if (m_DragonStage != DragonStage::Trial || !Visited(Region::DragonRoost))
		return false;
	m_DragonStage = DragonStage::Memory;
	return true;
}

bool QuestJournal::ReadDragonMemory()
{
	if (m_DragonStage != DragonStage::Memory || !Visited(Region::DragonRoost))
		return false;
	m_DragonStage = DragonStage::Complete;
	return true;
}

QuestJournal::DragonStage QuestJournal::DragonProgress() const
{
	return m_DragonStage;
}

int QuestJournal::DragonApproach() const
{
	return m_DragonApproach;
}

int QuestJournal::DragonChoice() const
{
	return m_DragonStage == DragonStage::Complete ? m_DragonApproach : 0;
}

bool QuestJournal::CanTravel(Region from, Region to) const
{
	if ((from == Region::StoneRidge && to == Region::DragonRoost) ||
	    (from == Region::DragonRoost && to == Region::StoneRidge))
		return DragonChoice() == 1 || DragonChoice() == 2;
	if (from == Region::Highlands && to == Region::DragonRoost)
		return m_DragonStage >= DragonStage::Audience;
	if ((from == Region::Basin && to == Region::DwarfMine) ||
	    (from == Region::DwarfMine && to == Region::Basin))
		return m_DwarfChoice == 1;
	if (from == Region::DwarfMine && to == Region::DwarfCity)
		return m_DwarfStage >= DwarfStage::Report;
	return true;
}

const QuestJournal::Stage& QuestJournal::TutorialStage() const
{
	return m_Stage;
}

const int& QuestJournal::LakeChoice() const
{
	return m_LakeChoice;
}

Region QuestJournal::NextDestination() const
{
	if (m_Stage != Stage::Complete)
		return Region::Village;
	if (m_DragonStage == DragonStage::Trace)
		return Region::Highlands;
	if (m_DragonStage >= DragonStage::Audience && m_DragonStage < DragonStage::Complete)
		return Region::DragonRoost;
	if (m_DwarfStage == DwarfStage::Record)
		return Region::DwarfMine;
	if (m_DwarfStage == DwarfStage::Report || m_DwarfStage == DwarfStage::Choice)
		return Region::DwarfCity;
	for (Region region : SurveyRegions)
		if (!Investigated(region))
			return region;
	if (m_DwarfStage == DwarfStage::Meeting)
		return Region::StoneRidge;
	return m_DragonStage == DragonStage::Meeting ? Region::Highlands : Region::None;
}

const wchar_t* QuestJournal::Objective() const
{
	if (m_Stage != Stage::Complete)
		return TutorialObjectives[static_cast<int>(m_Stage)];
	if (m_DragonStage == DragonStage::Trace)
		return L"바람등 고원 중앙의 벗겨진 비늘과 옛 경계문 조사하기";
	if (m_DragonStage == DragonStage::Audience)
		return L"잿빛 날개 쉼터에서 고룡 아르켄과 대화하기";
	if (m_DragonStage == DragonStage::Choice)
		return L"고룡에게 협상·회피 시험·거리 두기 중 접근 방식 선택하기";
	if (m_DragonStage == DragonStage::Trial)
		return L"고룡 앞에서 E로 시험 시작 · 세 번의 바닥 예고를 피해 버티기";
	if (m_DragonStage == DragonStage::Memory)
		return L"고룡에게 E로 봉인 이전의 기억 듣기";
	if (m_DwarfStage == DwarfStage::Record)
		return L"깊은돌 폐광길의 작업 기록 조사하기";
	if (m_DwarfStage == DwarfStage::Report)
		return L"깊은돌 도시의 기록관 브린에게 보고하기";
	if (m_DwarfStage == DwarfStage::Choice)
		return L"기록관과 대화하여 옛 교역길 개방 여부 결정하기";
	if (NextDestination() == Region::StoneRidge && Investigated(Region::StoneRidge))
		return L"돌등성이의 파수꾼 토르빅과 대화하기";
	switch (NextDestination())
	{
	case Region::Basin:
		return L"솔바람 분지의 이정표 조사하기";
	case Region::StoneRidge:
		return L"돌등성이 전초지의 드워프 표식 기록하기";
	case Region::MistShore:
		return L"안개 호숫가의 물결 표식 기록하기";
	case Region::Highlands:
		return L"바람등 고원의 길잡이 세라와 대화하기";
	default:
		return L"드워프와 고룡의 기록 완료 · 다음 여행은 해안과 미지의 존재로 이어집니다";
	}
}

std::vector<Region> QuestJournal::RouteToNext(Region from) const
{
	Region destination = NextDestination();
	if (destination == Region::None)
		return {};
	std::array<Region, RegionCount> previous = {};
	std::vector<Region> pending = {from};
	previous[static_cast<size_t>(from)] = from;
	for (size_t i = 0;
	     i < pending.size() && previous[static_cast<size_t>(destination)] == Region::None;
	     ++i)
		for (const auto& exit : WorldTravel::Exits(pending[i]))
		{
			if (!CanTravel(pending[i], exit.destination))
				continue;
			auto& parent = previous[static_cast<size_t>(exit.destination)];
			if (parent != Region::None)
				continue;
			parent = pending[i];
			pending.push_back(exit.destination);
		}
	if (previous[static_cast<size_t>(destination)] == Region::None)
		return {};
	std::vector<Region> route;
	for (Region at = destination;; at = previous[static_cast<size_t>(at)])
	{
		route.push_back(at);
		if (at == from)
			break;
	}
	std::reverse(route.begin(), route.end());
	return route;
}

std::vector<std::wstring> QuestJournal::Lines(int tab, Region current) const
{
	std::vector<std::wstring> lines;
	if (tab == 0)
	{
		lines.push_back(m_Stage == Stage::Complete ? L"의뢰 · 옛길의 이정표"
		                                           : L"의뢰 · 지도 밖의 첫 걸음");
		if (m_DwarfStage != DwarfStage::Meeting || Investigated(Region::StoneRidge))
			lines.push_back(L"드워프 이야기 · 돌에 새긴 귀환 약속");
		lines.push_back(Objective());
		lines.push_back(
		    L"고원은 분지 남쪽 길로 갈 수 있습니다. 드워프 이야기와 별개로 먼저 탐험해도 됩니다.");
		auto route = RouteToNext(current);
		if (route.size() == 1)
			lines.push_back(L"이 지역에 목표가 있습니다. 가까이에서 E로 조사·대화하세요.");
		else if (!route.empty())
		{
			lines.push_back(std::wstring(L"다음 출입구 목적지: ") + WorldTravel::Name(route[1]));
			std::wstring path = L"경로: ";
			for (Region region : route)
				path += (region == route.front() ? L"" : L" → ") +
				        std::wstring(WorldTravel::Name(region));
			lines.push_back(path);
		}
		lines.push_back(L"이정표 조사 위치는 각 야외 지역 지도의 황토색 점입니다.");
		lines.push_back(
		    L"조사 순서는 자유입니다. 도시 문은 작업 기록 확인 후, 분지 지름길은 개방 선택 후 이용합니다.");
	}
	else if (tab == 1)
	{
		static const wchar_t* clues[] = {
		    L"오래된 지도: 지금의 호수에 없는 길. 엘린의 부탁으로 빈칸을 기록하기 시작했다.",
		    L"숲지기의 단서: 표석과 동쪽 교역로에는 같은 드워프 양식의 표식이 남아 있다.",
		    L"표석: 건너온 이에게 길을, 돌아가는 이에게 표식을. 홈은 동쪽 교역로를 가리킨다.",
		    L"지도 조각: 교역로 건너 솔바람 분지, 세 갈래 길. 누군가는 이 길을 실제로 걸었다.",
		    L"미라의 꿈: 호수의 존재는 응답을 기다리는 듯하다. 정체는 아직 알 수 없다."};
		for (int i = 0; i < 5 && i < static_cast<int>(m_Stage); ++i)
			lines.push_back(clues[i]);
		lines.push_back(
		    m_LakeChoice == 1   ? L"선택 이력: 종을 울려 목소리에 응답했다. 호수의 빛이 돌아왔다."
		    : m_LakeChoice == 2 ? L"선택 이력: 종을 감싸 연결을 잠재웠다. 호수는 조용해졌다."
		                        : L"선택 이력: 아직 호수에서 선택하지 않았다.");
		if (m_Stage == Stage::Complete)
			lines.push_back(
			    L"마을 보고 완료: 엘린은 발견과 선택을 듣고 다음 여행을 위한 지도를 맡겼다.");
		const wchar_t* surveys[] = {
		    L"분지의 이정표: 교역로의 표식이 메아리 숲길까지 이어진다. 산맥과 물가로 갈라진다.",
		    L"돌등성이의 표식: 드워프 석공의 흔적이 산맥 쪽을 가리킨다. 후속 원정의 단서다.",
		    L"호숫가의 표식: 익숙한 물결과 빈 세 번째 홈. 멀리 해안과 고원으로 향하는 길의 흔적이다."};
		for (int i = 0; i < 3; ++i)
			if (Investigated(SurveyRegions[i]))
				lines.push_back(surveys[i]);
		if (m_DwarfStage >= DwarfStage::Record)
			lines.push_back(
			    L"토르빅의 부탁: 폐광의 자동 수호체는 침입자가 아니라 고장 난 길잡이다. 작업 기록을 찾아 달라.");
		if (m_DwarfStage >= DwarfStage::Report)
			lines.push_back(
			    L"작업 기록: 옛길은 사람과 물자를 위한 길이었다. 봉인은 침입보다 무분별한 채굴을 막으려 세워졌다.");
		if (m_DwarfStage >= DwarfStage::Choice)
			lines.push_back(
			    L"브린의 설명: 분지 지름길을 열면 교역을 재개하지만 경계 감시가 필요하다. 봉인을 유지해도 산길로 왕래할 수 있다.");
		if (m_DwarfChoice)
			lines.push_back(
			    m_DwarfChoice == 1
			        ? L"드워프 선택: 감시 아래 교역 재개. 솔바람 분지 ↔ 깊은돌 폐광길 지름길 개방. 도시는 여행자를 협력자로 맞이한다."
			        : L"드워프 선택: 옛길 봉인 유지. 지름길은 닫히고 돌등성이 산길은 유지. 도시는 안전 점검과 기록 보존에 집중한다.");
		if (m_DragonStage >= DragonStage::Trace)
			lines.push_back(
			    L"세라의 증언: 비늘은 싸움의 잔해가 아니라 탈피의 흔적이다. 고원 생물은 둥지와 열맥을 지킨다.");
		if (m_DragonStage >= DragonStage::Audience)
			lines.push_back(
			    L"고원의 경계문: 돌아가는 이에게 표식을. 날개와 드워프 인장이 나란히 새겨져 있다. 봉인은 공동의 약속이었다.");
		if (m_DragonStage == DragonStage::Trial)
			lines.push_back(
			    L"고룡의 시험: 힘보다 멈출 때를 배운다. 쉼터 안에서 세 번의 예고를 피하고 돌아와 기억을 듣는다. 중단하면 E로 재시도할 수 있다.");
		if (DragonChoice())
		{
			const wchar_t* choices[] = {
			    L"",
			    L"협상 · 서식지를 지키기로 약속. 돌등성이 바람길 개방.",
			    L"시험 · 세 번의 울림을 견뎌 경계 관찰자 인정. 돌등성이 바람길 개방.",
			    L"거리 두기 · 고룡의 권한을 받아들이지 않음. 바람길은 닫히고 고원 길은 유지."};
			lines.push_back(std::wstring(L"고룡 선택: ") + choices[DragonChoice()]);
			lines.push_back(
			    L"고룡의 기억: 봉인은 침입자를 가두는 벽이 아니라 서로의 응답을 늦추는 경계다. 통로를 열기 전에 양쪽의 뜻을 확인해야 한다.");
		}
	}
	else
	{
		lines.push_back(L"방문한 지역 / 조사한 이정표");
		for (size_t i = 1; i < RegionCount; ++i)
		{
			Region region = static_cast<Region>(i);
			if (Visited(region))
				lines.push_back(std::wstring(WorldTravel::Name(region)) +
				                (Investigated(region) ? L" · 조사 완료" : L" · 방문 기록"));
		}
		lines.push_back(L"직접 오간 연결 (같은 길은 한 번만 기록)");
		for (const auto& route : m_Routes)
			lines.push_back(std::wstring(WorldTravel::Name(route.first)) + L" ↔ " +
			                WorldTravel::Name(route.second));
	}
	return lines;
}

bool QuestJournal::IsOpen() const
{
	return m_Open;
}

void QuestJournal::Open()
{
	m_Open = true;
	m_Tab = m_Page = 0;
	m_PageCount = 1;
}

void QuestJournal::HandleKey(unsigned char key)
{
	if (key == 'j' || key == 27)
		m_Open = false;
	else if (key >= '1' && key <= '3')
	{
		m_Tab = key - '1';
		m_Page = 0;
	}
	else if (key == 'a')
		m_Page = std::max(0, m_Page - 1);
	else if (key == 'd')
		m_Page = std::min(m_PageCount - 1, m_Page + 1);
}

void QuestJournal::Draw(Renderer& r, int width, int height, Region current)
{
	if (!m_Open)
		return;
	const Color cream = {.94f, .90f, .76f, 1}, gold = {.98f, .73f, .36f, 1};
	float panelWidth = std::min(1000.f, width - 80.f), left = (width - panelWidth) * .5f;
	int characters = std::max(1, static_cast<int>((panelWidth - 64) / 19));
	int rowsPerPage = std::max(1, (height - 265) / 26);
	std::vector<std::wstring> rows;
	for (const auto& line : Lines(m_Tab, current))
		for (size_t i = 0; i < line.size(); i += characters)
			rows.push_back(line.substr(i, characters));
	m_PageCount = std::max(1, (static_cast<int>(rows.size()) + rowsPerPage - 1) / rowsPerPage);
	m_Page = std::min(m_Page, m_PageCount - 1);
	r.Rect(0, 0, static_cast<float>(width), static_cast<float>(height), {.015f, .025f, .03f, .85f});
	r.Rect(left, 40, panelWidth, height - 80.f, {.045f, .075f, .075f, .99f});
	r.Text(left + 26, 56, L"여행 일지 · " + std::wstring(WorldTravel::Name(current)), gold);
	const wchar_t* tabs[] = {L"[1] 목표", L"[2] 단서·선택", L"[3] 탐험 기록"};
	for (int i = 0; i < 3; ++i)
		r.Text(left + 26 + i * 235, 95, tabs[i], i == m_Tab ? gold : cream);
	for (int i = 0; i < rowsPerPage && m_Page * rowsPerPage + i < static_cast<int>(rows.size());
	     ++i)
		r.Text(left + 26, 145.f + i * 26, rows[m_Page * rowsPerPage + i], cream);
	r.Text(left + 26,
	       height - 100.f,
	       L"[A/D] 이전/다음 쪽   " + std::to_wstring(m_Page + 1) + L" / " +
	           std::to_wstring(m_PageCount) + L"    [J/Esc] 닫기",
	       gold);
	r.Text(left + 26,
	       height - 72.f,
	       L"이번 실행의 기록입니다. 일지를 보는 동안 이동과 전투가 멈춥니다.",
	       cream);
}
