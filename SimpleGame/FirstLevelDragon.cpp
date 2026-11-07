#include "stdafx.h"
#include "FirstLevel.h"
#include "QuestJournal.h"
#include <algorithm>
#include <cmath>

void FirstLevel::BuildDragonScene()
{
	if (m_Region == Region::Highlands)
	{
		m_StoryContact = &m_StaticActors.AddChild("Highlands/Sera", {55, 41});
		m_StoryContact->SetDraw(
		    [this](Actor& actor, Renderer& r)
		    {
			    Point p = Project(actor.WorldPosition(), m_Width, m_Height);
			    r.Character(p.x, p.y, 1, 8, {.52f, .62f, .46f, 1});
		    });
		m_DragonTrace = &m_StaticActors.AddChild("DragonTrace", WorldTravel::SurveyStone);
		ModelActor(*m_DragonTrace, LevelModel::Archive);
		ModelActor(m_DragonTrace->AddChild("ShedScale", {1, 0}), LevelModel::ScaleLoot);
	}
	else if (m_Region == Region::DragonRoost)
	{
		m_StoryContact = &m_StaticActors.AddChild("Dragon/Arken", WorldTravel::SurveyStone);
		ModelActor(*m_StoryContact, LevelModel::Dragon);
		m_StoryContact->SetUpdate(
		    [this](Actor&, float dt)
		    {
			    UpdateDragonTrial(dt);
		    });
		Actor& warning = m_StaticActors.AddChild("DragonTrial/Warning");
		warning.SetLayer(ActorLayer::Shadow);
		warning.SetDraw(
		    [this](Actor&, Renderer& r)
		    {
			    if (m_TrialWaves <= 0)
				    return;
			    Point p = Project(m_TrialTarget, m_Width, m_Height);
			    r.Ellipse(p.x, p.y, 76, 38, {1, .48f, .15f, .38f});
			    r.Ellipse(p.x,
			              p.y,
			              76 * (1 - m_TrialWindup / 1.5f),
			              38 * (1 - m_TrialWindup / 1.5f),
			              {1.2f, .72f, .22f, .5f});
		    });
	}
}

void FirstLevel::InteractDragon()
{
	using Stage = QuestJournal::DragonStage;
	m_DialoguePage = 0;
	m_StoryAction = StoryAction::None;
	Stage stage = m_Journal.DragonProgress();
	if (m_Region == Region::Highlands)
	{
		if (m_DragonTrace && std::hypot(m_Player.x - m_DragonTrace->WorldPosition().x,
		                                m_Player.y - m_DragonTrace->WorldPosition().y) < 2.5f)
		{
			if (stage == Stage::Trace)
			{
				m_Dialogue = {
				    L"비늘 곁의 경계문: 건너온 이에게 길을, 돌아가는 이에게 표식을. 드워프의 망치와 용의 날개가 같은 줄에 새겨져 있다.",
				    L"비늘에는 칼자국이 없다. 오래된 전쟁터라는 소문과 달리 이곳은 탈피와 휴식의 땅이었다. 불씨와 바람은 아직 열맥을 따라 맴돈다.",
				    L"바람이 잦아들며 쉼터로 가는 표식이 드러났다. 봉인은 한 종족이 다른 종족에게 세운 감옥이 아니었을지도 모른다."};
				m_StoryAction = StoryAction::DragonTrace;
			}
			else
				m_Dialogue = {
				    stage == Stage::Meeting
				        ? L"낡은 경계문과 벗겨진 비늘이다. 옆의 길잡이에게 먼저 이곳의 이야기를 물어보자."
				        : L"날개와 망치의 표식은 일지에 옮겼다. 쉼터로 가는 길은 열려 있다."};
		}
		else if (m_Journal.TutorialStage() != QuestJournal::Stage::Complete)
			m_Dialogue = {
			    L"길잡이 세라: 갈대여울의 지도를 먼저 완성해 줘. 그 뒤라면 이 경계문을 함께 읽어 볼 수 있겠어."};
		else if (stage == Stage::Meeting)
		{
			m_Dialogue = {
			    L"길잡이 세라: 용이 마을을 불태웠다는 소문을 들었니? 여기 남은 비늘은 상처가 아니라 탈피의 흔적이야. 싸움 전에 기록부터 보자.",
			    L"비늘도마뱀은 몸을 낮춘 뒤 돌진해. 바람 정령은 네가 서 있던 자리에 돌풍을 보내고, 열맥 불씨는 예고한 바닥을 잠시 달궈. 싸우지 않고 길을 따라가도 괜찮아.",
			    L"중앙 경계문에는 망치와 날개가 함께 새겨져 있어. 드워프와 아직 만나지 않았어도 읽을 수 있는 옛 약속이지. 조사한 뒤 동쪽 쉼터에서 아르켄을 만나 봐."};
			m_StoryAction = StoryAction::DragonStart;
		}
		else
			m_Dialogue = {
			    m_Journal.DragonChoice() == 3
			        ? L"세라: 거리를 두는 것도 선택이야. 바람길은 닫혔지만 분지로 돌아가는 길과 우리의 대화는 남아 있어."
			    : m_Journal.DragonChoice()
			        ? L"세라: 돌등성이로 향하는 바람길이 열렸어. 용의 기억도 지도에 적어 줘. 길을 안다는 건 그곳에 사는 이를 이해하는 일이겠지."
			        : L"세라: 경계문을 조사하면 동쪽 쉼터의 표식이 보여. 비늘 생물을 전부 고룡의 군대라고 생각하지 마. 이들은 각자의 터전을 지킬 뿐이야."};
		return;
	}
	if (stage == Stage::Audience)
	{
		m_Dialogue = {
		    L"고룡 아르켄: 작은 기록자여, 너는 벗겨진 껍질을 전쟁의 증거라 부르지 않았구나. 나는 문이 닫히기 전, 망치를 든 이들과 길을 지켰다.",
		    L"기억을 듣고 싶다면 서식지를 해치지 않겠다는 약속으로 협상하거나, 세 번의 울림 앞에서 물러설 때를 배우거라. 힘을 겨루어 나를 쓰러뜨릴 필요는 없다.",
		    L"내 권한을 받아들이지 않고 거리를 두어도 좋다. 그때는 경고만 전하고 바람길은 닫아 두겠다. 어느 쪽이든 고원을 통해 돌아갈 수 있다."};
		m_StoryAction = StoryAction::DragonAudience;
	}
	else if (stage == Stage::Choice)
		m_ChoosingDragon = true;
	else if (stage == Stage::Trial)
	{
		if (m_TrialWaves == 0)
			StartDragonTrial();
		else
			Message(L"시험 진행 중 · 쉼터 중앙 가까이에서 세 번의 바닥 예고를 피하세요.");
	}
	else if (stage == Stage::Memory)
	{
		m_Dialogue = {
		    m_Journal.DragonApproach() == 3
		        ? L"아르켄: 네가 내 판단에 동의하지 않더라도 이 경고만은 기록하라. 봉인을 허무는 일은 벽돌을 치우는 일이 아니다. 저편의 응답이 이쪽에 닿는 시간을 바꾸는 일이다."
		        : L"아르켄: 약속을 기억하겠다. 돌등성이로 향하는 바람길을 열어 두마. 오래전 드워프와 나는 길 양편의 목소리를 서로 들을 수 있게 했지.",
		    L"아르켄: 그러나 목소리가 겹치자 꿈과 현실의 길을 구별하지 못하는 이들이 생겼다. 봉인은 침입자를 가두는 벽이 아니라 서로의 응답을 늦추는 경계였다.",
		    L"지도에 날개와 망치, 아직 비어 있는 물결의 홈을 옮겼다. 물가에서 들은 목소리가 같은 약속의 다른 쪽일지 모른다. 그 정체를 지금 단정할 수는 없다."};
		m_StoryAction = StoryAction::DragonMemory;
	}
	else
		m_Dialogue = {
		    m_Journal.DragonChoice() == 3
		        ? L"아르켄: 서로 거리를 지키기로 했지. 바람길의 권한은 내게 남지만 고원으로 가는 길은 막지 않겠다. 내가 남긴 경고를 네 언어로 판단하라."
		    : m_Journal.DragonChoice() == 2
		        ? L"아르켄: 울림 앞에서 물러설 줄 아는 관찰자여. 힘으로 열 수 있는 문이라도 열지 않아야 할 때가 있다. 바람길은 네게 열려 있다."
		        : L"아르켄: 서식지를 지키기로 한 기록자여. 길을 열겠다는 약속은 그 뒤의 삶도 돌보겠다는 뜻이다. 바람길은 네게 열려 있다."};
}

void FirstLevel::StartDragonTrial()
{
	m_TrialWaves = 3;
	m_TrialWindup = 1.5f;
	m_TrialTarget = m_Player;
	Message(L"회피 시험 · 세 번의 울림, 주황색 바닥에서 벗어나세요. 중앙에서 멀어지면 중단됩니다.");
}

void FirstLevel::UpdateDragonTrial(float dt)
{
	if (m_TrialWaves <= 0)
		return;
	if (std::hypot(m_Player.x - WorldTravel::SurveyStone.x,
	               m_Player.y - WorldTravel::SurveyStone.y) > 8)
	{
		m_TrialWaves = 0;
		Message(L"쉼터에서 멀어져 시험을 중단했습니다. 고룡 앞에서 E로 다시 시작할 수 있습니다.");
		return;
	}
	m_TrialWindup -= dt;
	if (m_TrialWindup > 0)
		return;
	if (std::hypot(m_Player.x - m_TrialTarget.x, m_Player.y - m_TrialTarget.y) < 2.3f)
	{
		m_Hp = std::max(0, m_Hp - std::max(1, 12 - Defense()));
		m_HitCooldown = .6f;
		m_TrialWaves = 0;
		Message(L"울림에 닿아 시험을 중단했습니다. 바닥 예고를 보고 움직인 뒤 E로 재시도하세요.");
		return;
	}
	if (--m_TrialWaves > 0)
	{
		m_TrialWindup = 1.5f;
		m_TrialTarget = m_Player;
	}
	else if (m_Journal.CompleteDragonTrial())
		Message(L"세 번의 울림을 피했습니다. 고룡에게 E로 기억을 들으세요.");
}
