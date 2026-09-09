/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)
This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.
This program is distributed WITHOUT ANY WARRANTY.
*/
#include "stdafx.h"
#include "Renderer.h"
#include "Dependencies/freeglut.h"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <functional>
#include <iterator>

namespace
{
	const float Pi = 3.14159265f;
	const int MapWidth=152,MapHeight=132;
	const Color Cream = { .94f,.90f,.76f,1 }, Gold = { .98f,.73f,.36f,1 };
	const Color Teal = { .40f,.89f,.83f,1 }, Panel = { .035f,.075f,.085f,.94f };
	Renderer* renderer = nullptr;
	int width = 1280, height = 800;
	bool keys[256] = {}, showMap = false, paused = false;
	float elapsed = 0, lastTick = 0, completedAt = 0;
	Point player = { 18,50 }, camera = { 18,50 };
	bool walking = false, postprocess = true;
	int playerFacing = 1;
	enum Stage { Elder, Scout, Rune, Fragment, Fisher, Shrine, Return, Complete };
	Stage stage = Elder;
	int choice = 0; // 1: listen; 2: close. Session-local world history.
	bool choosing = false;
	std::vector<std::wstring> dialogue;
	size_t page = 0;
	std::wstring speaker;
	std::function<void()> afterDialogue;
	struct Person { Point p; const wchar_t* name; Color coat; };
	const Person people[] = {
		{{20,45},L"촌장 엘린",{.56f,.37f,.24f,1}},
		{{12,29},L"숲지기 로안",{.28f,.45f,.27f,1}},
		{{45,43},L"어부 미라",{.28f,.48f,.57f,1}},
		{{16,51},L"약초사 세나",{.53f,.54f,.30f,1}},
		{{25,49},L"대장장이 오렌",{.43f,.30f,.24f,1}},
		{{25,57},L"행상인 린",{.65f,.38f,.25f,1}},
		{{9,51},L"빵집 주인 노아",{.70f,.58f,.41f,1}},
		{{30,50},L"아이 유리",{.65f,.44f,.51f,1}},
		{{32,41},L"여행자 테오",{.41f,.39f,.60f,1}},
		{{19,55},L"직조공 아린",{.62f,.39f,.45f,1}},
		{{8,43},L"목수 베른",{.45f,.49f,.32f,1}},
		{{34,49},L"도공 루카",{.71f,.42f,.29f,1}},
		{{24,40},L"경비병 에다",{.37f,.43f,.50f,1}},
		{{40,55},L"양봉가 리오",{.70f,.60f,.27f,1}},
		{{63,69},L"농부 하나",{.46f,.55f,.30f,1}},
		{{76,83},L"순례자 이온",{.52f,.43f,.58f,1}},
		{{98,92},L"숯장이 오스",{.39f,.34f,.29f,1}},
		{{123,37},L"길잡이 레아",{.29f,.54f,.47f,1}}
	};
	const int PersonCount=(int)(sizeof(people)/sizeof(people[0]));
	struct House { Point p; float w,d; Color roof; };
	const House houses[] = {
		{{12,46},3,2.4f,{.57f,.26f,.18f,1}}, {{23,54},3.5f,2.5f,{.62f,.32f,.20f,1}},
		{{29,45},3,2.4f,{.42f,.27f,.23f,1}}, {{12,57},3,2.2f,{.64f,.39f,.22f,1}},
		{{32,56},2.8f,2.2f,{.45f,.30f,.25f,1}},
		{{39,59},3.1f,2.5f,{.58f,.35f,.22f,1}}, {{38,50},2.8f,2.4f,{.47f,.28f,.23f,1}},
		{{68,73},4,3,{.54f,.34f,.20f,1}}, {{102,96},3.2f,2.6f,{.42f,.29f,.21f,1}}
	};
	const int HouseCount=(int)(sizeof(houses)/sizeof(houses[0]));
	struct Tree { Point p; float size; int kind; };
	std::vector<Tree> trees;
	const Point route[] = { {18,50},{20,45},{12,29},{21,11},{34,13},{38,38},{45,43},{43,24} };
	const Point outerRoute[]={{20,49},{40,55},{62,64},{76,85},{106,96},{132,108}};
	const Point ridgeRoute[]={{62,64},{94,48},{124,36}};
	const Point ruins[]={{128,104},{131,103},{134,105},{129,110},{135,111}};
	const Point fires[]={{23,47},{37,52},{72,80},{100,90}};
	struct Animal { Point p,home; int kind; float phase,heading,speed; bool fleeing; };
	std::vector<Animal> animals;
	const Point animalHomes[]={{18,23},{30,21},{40,36},{59,56},{79,37},
		{92,67},{116,43},{128,54},{134,91},{112,112},{62,111},{35,94},{81,102},{143,116},{99,23}};
	const Point rune = {21,11}, fragment = {34,13}, shrine = {43,24};
	float Distance(Point a, Point b) { return std::hypot(a.x-b.x,a.y-b.y); }
	float SegmentDistance(Point p, Point a, Point b)
	{
		float x=b.x-a.x,y=b.y-a.y;
		float t=(p.x-a.x)*x+(p.y-a.y)*y;
		t=std::max(0.f,std::min(1.f,t/(x*x+y*y)));
		return Distance(p,{a.x+t*x,a.y+t*y});
	}
	bool OnPath(Point p)
	{
		for (int i=1;i<8;++i) if (SegmentDistance(p,route[i-1],route[i])<2.0f) return true;
		for(int i=1;i<6;++i)if(SegmentDistance(p,outerRoute[i-1],outerRoute[i])<1.7f)return true;
		for(int i=1;i<3;++i)if(SegmentDistance(p,ridgeRoute[i-1],ridgeRoute[i])<1.7f)return true;
		return SegmentDistance(p,{20,45},{45,43})<2.0f;
	}
	bool InLake(Point p, float margin = 0)
	{
		float x=(p.x-57)/(12+margin),y=(p.y-28)/(10+margin);
		return x*x+y*y<1;
	}
	Point Project(Point p, float z=0)
	{
		return { width*.5f+(p.x-p.y-camera.x+camera.y)*23,
			height*.53f+(p.x+p.y-camera.x-camera.y)*11.5f-z*23 };
	}
	Color Shade(Color c,float factor) { return {c.r*factor,c.g*factor,c.b*factor,c.a}; }
	void GroundQuad(float x,float y,float w,float d,Color c,float z=0,Material material=Material::Plain)
	{
		renderer->Polygon({Project({x,y},z),Project({x+w,y},z),Project({x+w,y+d},z),Project({x,y+d},z)},c,material);
	}
	void GroundEllipse(Point p,float rx,float ry,Color c,float z=0,Material material=Material::Plain)
	{
		std::vector<Point> points;
		int segments=material==Material::Water?96:64;
		for(int i=0;i<segments;++i) { float a=i*2*Pi/segments; points.push_back(Project({p.x+rx*std::cos(a),p.y+ry*std::sin(a)},z)); }
		renderer->Polygon(points,c,material);
	}
	void Box(Point p,float w,float d,float h,Color c,Material material=Material::Plain)
	{
		Point a={p.x-w/2,p.y-d/2},b={p.x+w/2,p.y-d/2},e={p.x+w/2,p.y+d/2},f={p.x-w/2,p.y+d/2};
		renderer->Polygon({Project(b),Project(e),Project(e,h),Project(b,h)},Shade(c,.65f),material,{{0,0},{d,0},{d,h},{0,h}});
		renderer->Polygon({Project(e),Project(f),Project(f,h),Project(e,h)},Shade(c,.84f),material,{{0,0},{w,0},{w,h},{0,h}});
		renderer->Polygon({Project(a,h),Project(b,h),Project(e,h),Project(f,h)},c,material,{{0,0},{w,0},{w,d},{0,d}});
	}
	void Stroke(Point a,Point b,float width,Color color)
	{
		float length=std::hypot(b.x-a.x,b.y-a.y);
		if(length<.001f)return;
		Point n={-(b.y-a.y)*width/(2*length),(b.x-a.x)*width/(2*length)};
		renderer->Polygon({{a.x+n.x,a.y+n.y},{b.x+n.x,b.y+n.y},{b.x-n.x,b.y-n.y},{a.x-n.x,a.y-n.y}},color);
	}
	void Talk(const wchar_t* name,std::initializer_list<std::wstring> pages,std::function<void()> done={})
	{
		speaker=name; dialogue=pages; page=0; afterDialogue=done;
		std::fill(std::begin(keys),std::end(keys),false);
	}
	Point Target()
	{
		switch(stage) { case Elder: case Return: return people[0].p; case Scout:return people[1].p;
		case Rune:return rune; case Fragment:return fragment; case Fisher:return people[2].p;
		case Shrine:return shrine; default:return player; }
	}
	const wchar_t* Objective()
	{
		const wchar_t* list[]={L"광장에서 촌장 엘린과 대화하기",L"숲길을 따라 숲지기 로안 만나기",L"숲 안쪽의 오래된 표석 조사하기",
			L"동쪽 공터에서 지도 조각 조사하기",L"호수 남쪽의 어부 미라 만나기",L"호수 서쪽의 돌 제단 조사하기",
			L"마을로 돌아가 엘린에게 이야기하기",L"첫 탐험 완료 · 마을과 호수를 자유롭게 둘러보세요"};
		return list[(int)stage];
	}
	int Nearby()
	{
		int result=-1; float best=2.5f;
		for(int i=0;i<PersonCount;++i) { float d=Distance(player,people[i].p); if(d<best) {best=d;result=i;} }
		const Point objects[]={rune,fragment,shrine};
		for(int i=0;i<3;++i) { float d=Distance(player,objects[i]); if(d<best) {best=d;result=PersonCount+i;} }
		return result;
	}
	void Interact()
	{
		int id=Nearby();
		if(id<0) return;
		if(id==0 && stage==Elder)
			Talk(L"촌장 엘린",{L"갈대여울에 온 걸 환영하네. 목적지가 정해지지 않은 여행도 좋지. 길을 걷다 보면 이유는 생기니까.",
			L"며칠 전, 물에 젖은 오래된 지도를 얻었네. 그런데 지금의 호수에 없는 길이 그려져 있어.",
			L"북쪽 숲지기 로안이라면 그 표식을 알지도 모르네. 숲의 표석과 호숫가를 살펴봐 주겠나?",
			L"서두르지 않아도 돼. 주민들의 이야기도 들어 보게. WASD로 걷고, 가까이 다가가 E로 말을 걸면 되네."},[]{stage=Scout;});
		else if(id==1 && stage==Scout)
			Talk(L"숲지기 로안",{L"그 지도에 새겨진 모양… 숲 안쪽 표석에서도 봤어. 오래된 길을 따라 북쪽으로 가 봐.",
			L"드워프가 만들었다는 소문이 있지만 확실하진 않아. 누가 만들었는지는 직접 흔적을 보고 판단해야지.",
			L"표석 옆에는 버려진 야영지도 있어. 나무 사이의 불빛을 따라가면 길을 잃지 않을 거야. M으로 지도를 볼 수도 있어."},[]{stage=Rune;});
		else if(id==PersonCount && stage==Rune)
			Talk(L"오래된 표석",{L"이끼 아래에 세 개의 물결과 문처럼 생긴 무늬가 있다. 정교한 홈은 자연적으로 생긴 것이 아니다.",
			L"무너진 문 밑의 글자 중 일부만 읽힌다. '건너온 이에게 길을…' 누군가 이곳을 통로로 사용했던 걸까?",
			L"동쪽 공터의 천 조각에도 같은 무늬가 보인다. 그곳을 살펴보자."},[]{stage=Fragment;});
		else if(id==PersonCount+1 && stage==Fragment)
			Talk(L"젖은 지도 조각",{L"버려진 야영지에서 지도의 나머지 조각을 찾았다. 선을 맞추자 길은 호수 한가운데까지 이어진다.",
			L"가장자리에 적힌 메모: '물 아래에서도 대답이 들린다. 이름을 묻기 전에, 먼저 들어라.'",
			L"이곳에 온 사람은 보물을 찾았던 걸까, 누군가를 만나려 했던 걸까? 남쪽 호숫가의 어부에게 물어보자."},[]{stage=Fisher;});
		else if(id==2 && stage==Fisher)
			Talk(L"어부 미라",{L"나도 그 꿈을 꿨어. 낯선 해안에 서 있는데 누군가 내 말을 기다리고 있었지. 무섭지만 외로운 느낌이었어.",
			L"서쪽 돌 제단의 종이 바람 없이 울릴 때가 있어. 소리를 되돌려 주면 뭔가 일어날지도 몰라.",
			L"응답할지, 종을 잠재울지는 네가 정해. 우린 아직 저 존재가 누군지 모르니까. 네 선택을 마을에도 알려 줘."},[]{stage=Shrine;});
		else if(id==PersonCount+2 && stage==Shrine)
			Talk(L"호숫가의 제단",{L"물속에서 별빛 같은 점들이 모인다. 소리는 말이 아니라 질문처럼 느껴진다. '거기… 누가 있니?'",
			L"제단에는 작은 종과 종을 감쌀 천이 놓여 있다. 응답하면 연결이 열리고, 감싸면 지금의 고요를 지킬 수 있을 듯하다.",
			L"정답은 아직 모른다. 다만 이곳에서의 선택은 호수와 마을에 흔적을 남길 것이다."},[]{choosing=true;});
		else if(id==0 && stage==Return)
			Talk(L"촌장 엘린",{choice==1?L"호수에 불빛이 돌아왔군. 미라도 꿈속의 소리가 한결 가까워졌다고 하더군. 네가 문을 두드린 모양이야.":L"호수가 조용해졌군. 미라는 오늘 오랜만에 푹 잘 수 있을 것 같다고 하더군. 네가 잠시 거리를 두기로 했구나.",
			L"우리가 아는 전설과 실제 모습은 다를 수 있겠지. 네가 본 것과 선택한 일은 이 마을이 기억하겠네.",
			L"지도는 가지고 가게. 아직 빈칸이 많으니까. 오늘의 여행은 여기까지지만, 세상에는 알아갈 존재가 더 있네."},[]{stage=Complete; completedAt=elapsed;});
		else if(id==0) Talk(people[id].name,{choice?L"네가 전한 이야기를 이웃들과 나누고 있네. 같은 호수도 이제 다르게 보이는군.":L"숲길은 마을 북쪽에 있네. 지도에 표시된 길을 따라가 보게. 서두를 필요는 없어."});
		else if(id==1) Talk(people[id].name,{L"표석에서 야영지까지 길은 이어져 있어. 숲에서 발견한 것이 있다면 미라에게도 들려줘."});
		else if(id==2) Talk(people[id].name,{choice==1?L"불빛이 내 노를 따라오네. 저쪽에서도 우리를 궁금해하는 걸까?":choice==2?L"오늘은 물결 소리만 들려. 당분간은 이 고요를 즐기고 싶어.":L"호수 한가운데에 길이 있었다는 옛이야기를 들어 봤니? 숲에서 내려온 돌들이 물속에도 있대."});
		else if(id==3) Talk(people[id].name,{L"여행자는 돌아올 힘도 남겨 두어야 해. 이 세계에서 회복과 부활에는 대가가 따르니까.",L"오늘의 숲길에는 싸울 일이 없을 거야. 천천히 둘러보고 와. 자세한 치료 규칙은 이후의 여행에서 배우게 될 거야."});
		else if(id==4) Talk(people[id].name,{L"저 표석을 쪼갠 도구는 여기 것과 달라. 드워프의 솜씨라고들 하지만, 나는 아직 그들을 만나 본 적 없어."});
		else if(id==5) Talk(people[id].name,{L"고원에서는 용 그림자를 봤다는 이야기도 들려. 어디까지 믿어야 할까? 언젠가 직접 가 보고 싶네."});
		else if(id==6) Talk(people[id].name,{L"새벽에 구운 빵 냄새가 숲까지 닿는다던데. 길을 잃으면 냄새를 따라 돌아오렴."});
		else if(id==7) Talk(people[id].name,{choice==1?L"봤어? 호수에 별이 내려왔어! 네가 불렀다며?":choice==2?L"호수가 잠든 것 같아. 오늘은 꿈속에서 누가 부르지 않을까?":L"호수에서 돌이 노래하는 걸 들었어. 어른들은 개구리라는데, 개구리가 내 이름도 알까?"});
		else if(id==8) Talk(people[id].name,{L"나는 내일 산맥으로 떠나. 너는 어느 길을 택할 거야? 여행길에서 다시 만날지도 모르지."});
		else if(id>=9 && id<PersonCount)
		{
			const wchar_t* stories[]={L"여행용 망토를 짜고 있어. 오래된 돌길에서도 신발과 옷은 닳기 마련이지.",
				L"마을 집들은 돌 기초 위에 참나무 골조를 세워 지었어. 숲의 목재도 함부로 쓰지 않아.",
				L"호수 진흙으로 빚은 그릇에는 가끔 이상한 무늬가 떠올라. 물 아래에 무엇이 있는 걸까?",
				L"동쪽 길이 다시 열렸어. 숲의 짐승은 먼저 거리를 두면 대개 제 갈 길로 가더군.",
				L"동쪽 초원에는 사슴과 토끼가 많아. 가까이 달려들면 놀라서 숨으니 천천히 지켜봐.",
				L"이 밭 너머로 순례자의 쉼터가 있어. 마을에서 오는 오래된 길을 따라가면 돼.",
				L"여정은 멀리 가는 것만으로 깊어지지 않지. 오늘 본 것에 어떤 답을 했는지가 남는 법이야.",
				L"모닥불은 자리를 떠나기 전에 돌봐야 해. 동쪽 폐허에는 멧돼지들이 드나들어.",
				L"여기서 서쪽을 보면 거울 호수가 보여. 숲의 짐승들도 그 물빛에 반응하는 걸까?"};
			Talk(people[id].name,{stories[id-9]});
		}
		else Talk(L"조사",{L"낯선 표식이 남아 있다. 먼저 사람들에게 이야기를 듣고 단서를 연결해 보자."});
	}
	void MakeChoice(int value)
	{
		choice=value; choosing=false; stage=Return;
		Talk(L"세계에 남긴 흔적",{value==1?L"종을 울렸다. 물 아래에서 불빛이 피어나고, 먼 곳의 소리가 한 번 더 응답한다. 호수와의 연결이 열렸다.":L"종을 천으로 감쌌다. 물 위의 빛이 천천히 가라앉는다. 호수와의 연결을 잠재웠다.",L"마을로 돌아가 촌장 엘린에게 발견과 선택을 전하자."});
	}
	bool CanWalk(Point p)
	{
		if(p.x<2 || p.y<2 || p.x>MapWidth-2 || p.y>MapHeight-2 || InLake(p,.5f)) return false;
		for(const auto& h:houses) if(std::abs(p.x-h.p.x)<h.w/2+.4f && std::abs(p.y-h.p.y)<h.d/2+.4f) return false;
		for(const auto& tree:trees) if(Distance(p,tree.p)<.6f) return false;
		for(Point ruin:ruins)if(Distance(p,ruin)<.85f)return false;
		return true;
	}
	void UpdateAnimals(float dt)
	{
		for(auto& a:animals)
		{
			Point desired={a.home.x+std::cos(elapsed*.17f+a.phase)*5,a.home.y+std::sin(elapsed*.13f+a.phase)*4};
			a.fleeing=Distance(a.p,player)<(a.kind==1?5.f:7.f);
			if(a.fleeing)desired={a.p.x+(a.p.x-player.x)*3,a.p.y+(a.p.y-player.y)*3};
			if(Distance(a.p,a.home)>15)desired=a.home;
			float angle=std::atan2(desired.y-a.p.y,desired.x-a.p.x);
			float speed=a.fleeing?4.3f:1.0f;
			if(!a.fleeing && std::sin(elapsed*.7f+a.phase)>.45f)speed=0;
			a.speed=0;
			if(speed==0)continue;
			// Try alternate headings instead of walking through trunks or into the lake.
			const float turns[]={0,.65f,-.65f,1.3f,-1.3f,2.1f,-2.1f};
			for(float turn:turns)
			{
				float heading=angle+turn;Point step={std::cos(heading)*speed*dt,std::sin(heading)*speed*dt};
				Point next={a.p.x+step.x,a.p.y+step.y};
				if(CanWalk(next) && Distance(next,player)>.8f)
				{a.p=next;a.heading=heading;a.speed=speed;break;}
			}
		}
	}
	bool Visible(Point p,float margin=180)
	{
		Point s=Project(p);return s.x>-margin&&s.x<width+margin&&s.y>-margin&&s.y<height+margin;
	}
	void CastShadow(Point p,float radius,float objectHeight)
	{
		if(!Visible(p))return;
		for(int i=5;i>=1;--i)
		{
			float t=i/5.f;
			Point s=Project({p.x+objectHeight*.55f*t,p.y+objectHeight*.18f*t});
			renderer->SoftEllipse(s.x,s.y,radius*25*(1-.18f*t),radius*12,{.025f,.045f,.045f,.13f});
		}
		Point s=Project(p);
		renderer->SoftEllipse(s.x,s.y,radius*20,radius*9,{.02f,.025f,.025f,.30f});
	}
	void DrawAnimal(const Animal& a)
	{
		Point p=Project(a.p);float sign=std::cos(a.heading)-std::sin(a.heading)>=0?1.f:-1.f;
		float size=a.kind==1?.65f:1.f, gait=a.speed>0?std::sin(elapsed*(a.fleeing?16:8)+a.phase):0;
		float bob=a.kind==1?std::abs(gait)*4:std::abs(gait);
		Color fur=a.kind==0?Color{.59f,.40f,.23f,1}:a.kind==1?Color{.66f,.61f,.48f,1}:Color{.31f,.28f,.23f,1};
		for(int leg=0;leg<4;++leg)
		{
			float x=p.x+((leg%2)*20-10)*size+(leg/2)*4;
			float move=gait*(leg%2==leg/2?1:-1)*5*size;
			renderer->Polygon({{x,p.y-17*size},{x+4*size,p.y-17*size},{x+move+3*size,p.y},{x+move,p.y}},Shade(fur,.62f));
		}
		renderer->Ellipse(p.x,p.y-21*size-bob,20*size,11*size,fur);
		renderer->Ellipse(p.x-4,p.y-25*size-bob,12*size,6*size,Shade(fur,1.17f));
		float hx=p.x+sign*20*size,hy=p.y-(a.kind==0?38:26)*size-bob;
		renderer->Polygon({{p.x+sign*10*size,p.y-18*size},{hx+sign*5*size,hy},{hx-sign*6*size,hy-4},{p.x+sign*5*size,p.y-28*size}},fur);
		renderer->Ellipse(hx,hy,9*size,7*size,fur);
		renderer->Ellipse(hx+sign*7*size,hy+2*size,4*size,3*size,Shade(fur,.63f));
		renderer->Ellipse(hx+sign*3*size,hy-2*size,1.5f,1.5f,{.05f,.055f,.04f,1});
		if(a.kind==0)
		{
			for(int i=0;i<2;++i)
			{
				float x=hx-4+i*7;
				renderer->Polygon({{x,hy-5},{x-3,hy-22},{x-1,hy-22},{x+2,hy-5}},Cream);
				renderer->Polygon({{x-1,hy-14},{x-7,hy-20},{x-6,hy-16},{x,hy-11}},Cream);
			}
		}
		else if(a.kind==1)
		{
			renderer->Ellipse(hx-3,hy-11,3,10,fur);renderer->Ellipse(hx+3,hy-12,3,11,fur);
			renderer->Ellipse(p.x-sign*18*size,p.y-20*size,5,5,Cream);
		}
		else
		{
			renderer->Polygon({{hx+sign*6,hy+4},{hx+sign*12,hy-3},{hx+sign*10,hy+7}},Cream);
			renderer->Polygon({{p.x-13,p.y-30},{p.x-8,p.y-36},{p.x+12,p.y-30}},Shade(fur,.7f));
		}
	}
	void DrawFire(Point p)
	{
		Point s=Project(p);float flicker=std::sin(elapsed*9+p.x)*3;
		GroundEllipse(p,1.5f,1.2f,{.18f,.17f,.13f,1},0,Material::Stone);
		Box(p,1.2f,.3f,.24f,{.31f,.18f,.085f,1},Material::Wood);
		renderer->SoftEllipse(s.x,s.y-8,88+flicker,52,{2.0f,.72f,.16f,.18f});
		for(int i=0;i<4;++i)
		{
			float x=s.x+(i-1.5f)*5,y=s.y-4;
			float tip=31+std::sin(elapsed*7+i*2+p.x)*9;
			renderer->Polygon({{x-8,y},{x-4+flicker,y-tip*.6f},{x+std::sin(elapsed*6+i)*6,y-tip},{x+9,y}}, {2.4f,.57f,.09f,.75f});
			renderer->Polygon({{x-4,y},{x+2,y-tip*.6f},{x+5,y}}, {3.0f,1.8f,.48f,.95f});
		}
		for(int i=0;i<12;++i)
		{
			float age=std::fmod(elapsed*.6f+i*.0833f,1.f);
			renderer->Ellipse(s.x+std::sin(i*3.f+elapsed)*age*20,s.y-20-age*64,1.4f,2,{2.1f,1.1f,.3f,(1-age)*.8f});
		}
		for(int i=0;i<4;++i)
		{
			float age=std::fmod(elapsed*.22f+i*.25f,1.f);
			renderer->SoftEllipse(s.x+age*20,s.y-38-age*70,8+age*22,12+age*14,{.39f,.39f,.32f,(1-age)*.2f});
		}
	}
	void DrawHouse(const House& h)
	{
		Color timber={.27f,.17f,.09f,1},plaster={.80f,.73f,.56f,1};
		Box(h.p,h.w,h.d,2.7f,plaster,Material::Plaster);
		Box(h.p,h.w+.12f,h.d+.12f,.38f,{.45f,.44f,.34f,1},Material::Stone);
		// Facade coordinates are in the wall plane, including doors and shutters.
		auto front=[&](float x,float z){return Project({h.p.x+x,h.p.y+h.d*.5f},z);};
		auto panel=[&](float x,float z,float w,float height,Color color,Material material)
		{
			renderer->Polygon({front(x,z),front(x+w,z),front(x+w,z+height),front(x,z+height)},color,material,{{0,0},{w,0},{w,height},{0,height}});
		};
		for(int i=-1;i<=1;++i)Stroke(front(i*h.w*.48f,.35f),front(i*h.w*.48f,2.7f),4,timber);
		for(float z:{.48f,2.5f})Stroke(front(-h.w*.5f,z),front(h.w*.5f,z),4,timber);
		Stroke(front(-h.w*.47f,1.7f),front(-h.w*.28f,2.5f),3,timber);
		Stroke(front(h.w*.47f,1.7f),front(h.w*.28f,2.5f),3,timber);
		for(int i=0;i<3;++i)
		{
			Point post={h.p.x+h.w*.5f,h.p.y+h.d*(i*.5f-.5f)};
			Stroke(Project(post,.35f),Project(post,2.7f),4,timber);
		}
		Stroke(Project({h.p.x+h.w*.5f,h.p.y-h.d*.5f},2.5f),Project({h.p.x+h.w*.5f,h.p.y+h.d*.5f},2.5f),4,timber);
		panel(-.54f,.38f,1.08f,1.85f,timber,Material::Wood);
		panel(-.42f,.4f,.84f,1.65f,{.40f,.27f,.13f,1},Material::Wood);
		Point handle=front(.27f,1.15f);renderer->Ellipse(handle.x,handle.y,2,2,{.72f,.56f,.25f,1});
		GroundQuad(h.p.x-.62f,h.p.y+h.d*.5f,1.24f,.55f,{.51f,.49f,.40f,1},.14f,Material::Stone);
		for(int side:{-1,1})
		{
			float x=side*(h.w*.25f+.27f)-.22f;
			panel(x-.06f,1.1f,.56f,1.05f,timber,Material::Wood);
			panel(x,1.2f,.44f,.83f,{1.25f,.82f,.32f,1},Material::Plain);
			panel(x-.20f,1.17f,.14f,.88f,{.33f,.39f,.25f,1},Material::Wood);
			panel(x+.50f,1.17f,.14f,.88f,{.33f,.39f,.25f,1},Material::Wood);
			Stroke(front(x+.22f,1.2f),front(x+.22f,2.03f),2,timber);
			Stroke(front(x,1.61f),front(x+.44f,1.61f),2,timber);
			Stroke(front(x-.08f,1.08f),front(x+.52f,1.08f),5,timber);
		}
		Point a={h.p.x-h.w*.6f,h.p.y-h.d*.6f},b={h.p.x+h.w*.6f,h.p.y-h.d*.6f};
		Point c={h.p.x+h.w*.6f,h.p.y+h.d*.6f},d={h.p.x-h.w*.6f,h.p.y+h.d*.6f};
		Point r1={h.p.x,h.p.y-h.d*.6f},r2={h.p.x,h.p.y+h.d*.6f};
		float slope=std::hypot(h.w*.6f,1.5f),length=h.d*1.2f;
		renderer->Polygon({Project(a,2.7f),Project(r1,4.2f),Project(r2,4.2f),Project(d,2.7f)},h.roof,Material::Roof,{{0,0},{0,slope},{length,slope},{length,0}});
		renderer->Polygon({Project(r1,4.2f),Project(b,2.7f),Project(c,2.7f),Project(r2,4.2f)},Shade(h.roof,.72f),Material::Roof,{{0,slope},{0,0},{length,0},{length,slope}});
		renderer->Polygon({Project(d,2.7f),Project(c,2.7f),Project(r2,4.2f)},plaster,Material::Plaster,{{0,0},{h.w*1.2f,0},{h.w*.6f,1.5f}});
		Stroke(Project(d,2.7f),Project(c,2.7f),5,timber);
		Stroke(Project(d,2.7f),Project(r2,4.2f),5,timber);
		Stroke(Project(c,2.7f),Project(r2,4.2f),5,timber);
		Stroke(Project(r2,2.7f),Project(r2,4.2f),3,timber);
		Stroke(Project(r1,4.2f),Project(r2,4.2f),5,Shade(h.roof,1.2f));
		Stroke(Project(b,2.7f),Project(c,2.7f),5,timber);
		// Only the chimney section above the roof is visible.
		Point chimney={h.p.x+h.w*.25f,h.p.y-h.d*.15f};
		float baseLeft=4.2f-1.5f*.25f/.6f,baseRight=4.2f-1.5f*(h.w*.25f+.55f)/(h.w*.6f);
		Point ca=Project({chimney.x,chimney.y+.55f},baseLeft),cb=Project({chimney.x+.55f,chimney.y},baseRight);
		Point cc=Project({chimney.x+.55f,chimney.y+.55f},baseRight);
		Point ct=Project({chimney.x+.55f,chimney.y+.55f},4.75f);
		renderer->Polygon({cb,cc,ct,Project({chimney.x+.55f,chimney.y},4.75f)},{.40f,.38f,.30f,1},Material::Stone,{{0,0},{.55f,0},{.55f,1.1f},{0,1.1f}});
		renderer->Polygon({ca,cc,ct,Project({chimney.x,chimney.y+.55f},4.75f)},{.53f,.50f,.39f,1},Material::Stone,{{0,0},{.55f,0},{.55f,1.1f},{0,1.1f}});
		GroundQuad(chimney.x-.08f,chimney.y-.08f,.71f,.71f,{.32f,.30f,.25f,1},4.75f,Material::Stone);
		Point smoke=Project({chimney.x+.27f,chimney.y+.27f},4.85f);
		for(int i=0;i<5;++i)
		{
			float age=std::fmod(elapsed*.18f+i*.2f,1.f);
			renderer->SoftEllipse(smoke.x+age*24+std::sin(age*6+h.p.x)*4,smoke.y-age*60,5+age*16,8+age*15,{.75f,.76f,.68f,(1-age)*.17f});
		}
	}
	void DrawTree(const Tree& t)
	{
		Point p=Project(t.p), hero=Project(player);
		// Multiple overlapping leaf layers need lower per-layer opacity near the player.
		float alpha=(p.y>hero.y && std::abs(p.x-hero.x)<55*t.size && p.y-hero.y<145*t.size) ? .10f:1.f;

		Color bark={.30f,.23f,.14f,alpha};
		Box(t.p,.34f,.34f,2.5f*t.size,bark,Material::Wood);
		for(int i=-1;i<=1;++i)Stroke({p.x,p.y-15*t.size},{p.x+i*9*t.size,p.y+2},3*t.size,bark);
		float sway=std::sin(elapsed*.8f+t.p.x)*1.5f*t.size;
		if(t.kind==0)
			for(int i=0;i<5;++i)
			{
				float y=p.y-(30+i*17)*t.size,w=(36-i*5.5f)*t.size,x=p.x+sway*(i+1)*.2f;
				Color needles={.105f+i*.015f,.235f+i*.023f,.16f+i*.012f,alpha};
				for(int j=-2;j<=2;++j)
				{
					float bx=x+j*w*.32f,by=y-std::abs(j)*5*t.size;
					renderer->Polygon({{bx-w*.4f,by+7*t.size},{x,y-43*t.size},{bx+w*.4f,by+4*t.size}},needles,Material::Foliage,{{-w,0},{0,43*t.size},{w,0}});
				}
				renderer->Polygon({{x-w*.72f,y-4*t.size},{x,y-43*t.size},{x+w*.05f,y-9*t.size}},Shade(needles,1.18f),Material::Foliage,{{-w,0},{0,43*t.size},{0,0}});
			}
		else
		{
			for(int i=-1;i<=1;++i)Stroke({p.x,p.y-26*t.size},{p.x+i*24*t.size,p.y-(70-std::abs(i)*10)*t.size},5*t.size,bark);
			for(int i=0;i<13;++i)
			{
				float angle=i*2.4f+t.p.x,ring=i<9?1.f:.48f;
				float x=p.x+sway+std::cos(angle)*25*t.size*ring;
				float y=p.y-76*t.size+std::sin(angle)*26*t.size*ring;
				float radius=(17+3*std::sin(i*7.f+t.p.y))*t.size;
				float light=.88f-.12f*std::cos(angle)-.14f*std::sin(angle);
				std::vector<Point> outline,uv;
				for(int j=0;j<16;++j)
				{
					float a=j*2*Pi/16;Point v={std::cos(a)*radius,std::sin(a)*radius*.85f};
					outline.push_back({x+v.x,y+v.y});uv.push_back({v.x+i*13,v.y});
				}
				renderer->Polygon(outline,Shade({.32f,.43f,.22f,alpha},light),Material::Foliage,uv);
			}
		}
	}
	void DrawPerson(Point location,Color coat,bool hero,int index)
	{
		Point p=Project(location);
		if(hero)renderer->SoftEllipse(p.x,p.y,17,8,{.75f,.88f,.67f,.36f});
		float breath=hero&&walking?0:std::sin(elapsed*1.8f+index)*.65f;
		int frame=hero&&walking?(int)(elapsed*10)%8:8;
		renderer->Character(p.x,p.y+breath,hero?playerFacing:index%4,frame,coat);
	}
	void DrawRelic(int id)
	{
		Point p=id==0?rune:id==1?fragment:shrine;
		if(id==1)
		{
			Box(p,1.5f,1,.35f,{.39f,.29f,.18f,1});
			GroundQuad(p.x-.5f,p.y-.35f,1,.7f,Cream,.4f); return;
		}
		GroundEllipse(p,2,1.8f,{.18f,.27f,.24f,1});
		Box(p,1.2f,.9f,id==0?2.3f:1.25f,{.46f,.54f,.47f,1},Material::Stone);
		Point top=Project(p,id==0?1.6f:1.6f);
		Color light=choice==2 && id==2?Shade(Cream,.6f):Teal;
		renderer->SoftEllipse(top.x,top.y,35,42,{light.r,light.g,light.b,.26f});
		renderer->Polygon({{top.x,top.y-10},{top.x+5,top.y},{top.x,top.y+10},{top.x-5,top.y}},light);
		if(id==2 && choice==2) renderer->Rect(top.x-9,top.y-5,18,10,{.45f,.35f,.27f,1});
	}
	void DrawPath(const Point* points,int count,float radius)
	{
		Color earth={.48f,.39f,.25f,1};
		for(int i=1;i<count;++i)
		{
			Point a=points[i-1],b=points[i];float length=Distance(a,b);
			Point n={-(b.y-a.y)/length*radius,(b.x-a.x)/length*radius};
			renderer->Polygon({Project({a.x+n.x,a.y+n.y}),Project({b.x+n.x,b.y+n.y}),
				Project({b.x-n.x,b.y-n.y}),Project({a.x-n.x,a.y-n.y})},earth,Material::Dirt);
			GroundEllipse(b,radius,radius,earth,0,Material::Dirt);
		}
	}
	void World()
	{
		for(int y=0;y<MapHeight;y+=2)for(int x=0;x<MapWidth;x+=2)
		{
			Point center={(float)x+1,(float)y+1};if(!Visible(center,90))continue;
			float shade=std::sin(x*.05f)*.024f+std::cos(y*.07f)*.02f;
			GroundQuad((float)x,(float)y,2,2,{.28f+shade,.38f+shade,.23f,1},0,Material::Grass);
		}
		DrawPath(route,8,1.65f);DrawPath(outerRoute,6,1.4f);DrawPath(ridgeRoute,3,1.4f);
		const Point returnPath[]={{20,45},{45,43}};DrawPath(returnPath,2,1.6f);
		GroundEllipse({20,49},10,8,{.53f,.49f,.39f,1},0,Material::Stone);
		GroundEllipse({132,107},8,7,{.40f,.43f,.34f,1},0,Material::Stone);
		GroundEllipse({100,91},5,4,{.46f,.38f,.26f,1},0,Material::Dirt);
		// Tilled strips beside the eastern farmhouse.
		for(int row=0;row<8;++row)
		{
			GroundQuad(63+row*.75f,77,.55f,8,{.28f,.22f,.14f,1},0,Material::Dirt);
			for(int k=0;k<10;++k)
			{
				Point crop=Project({63.2f+row*.75f,77.4f+k*.7f});
				renderer->Polygon({{crop.x-3,crop.y},{crop.x,crop.y-9},{crop.x+3,crop.y}}, {.52f,.56f,.25f,1});
			}
		}
		// Water material owns depth coloration, animated caustics and shoreline foam.
		GroundEllipse({57,28},13.3f,11.3f,{.57f,.53f,.36f,1},0,Material::Dirt);
		GroundEllipse({57,28},12.8f,10.8f,{.25f,.46f,.38f,1});
		GroundEllipse({57,28},12,10,{.20f,.46f,.43f,1},0,Material::Water);
		for(int i=0;i<28;++i)
		{
			float a=i*2*Pi/28+.035f*std::sin(i*9.f);
			Point p={57+std::cos(a)*12.65f,28+std::sin(a)*10.65f};
			if(!Visible(p))continue;
			GroundEllipse(p,.35f+.18f*(i%3),.24f,{.35f,.40f,.30f,1},0,Material::Stone);
			Point s=Project(p);
			for(int j=0;j<4;++j)
			{
				float x=s.x+(j-1.5f)*4,y=s.y+std::sin(j*3.f+i)*3;
				float sway=std::sin(elapsed*1.3f+i+j*.4f)*2,tip=12+(i+j)%4*4.f;
				Stroke({x,y},{x+sway,y-tip},1.4f,{.33f,.39f,.19f,1});
				renderer->Polygon({{x,y-3},{x-5+sway,y-tip*.8f},{x+1,y-6}},{.41f,.46f,.23f,1});
				if(j%2==0)Stroke({x+sway,y-tip},{x+sway,y-tip+5},3,{.36f,.27f,.14f,1});
			}
		}
		if(choice!=2)
			for(int i=0;i<(choice==1?36:8);++i)
			{
				Point w={55+std::sin(i*2.4f)*7,28+std::cos(i*1.7f)*6};
				Point s=Project(w,.12f+std::sin(elapsed+i)*.1f);
				renderer->SoftEllipse(s.x,s.y,25,12,{.3f,1.5f,1.35f,.20f});
				renderer->Ellipse(s.x,s.y,2,2,{.8f,2.2f,1.9f,.85f});
			}
		for(int i=0;i<7;++i)GroundQuad(44+i*.25f,39,.22f,2,{.48f,.33f,.19f,1},.08f,Material::Wood);
		for(int i=0;i<700;++i)
		{
			Point p={3+(float)((i*29)%(MapWidth-6)),3+(float)((i*43)%(MapHeight-6))};
			if(!Visible(p)||InLake(p,1)||OnPath(p)||Distance(p,{20,49})<10)continue;
			Point s=Project(p);
			renderer->Ellipse(s.x,s.y,2,2,i%3?Color{.64f,.62f,.32f,.7f}:Color{.76f,.62f,.57f,.8f});
		}
		// Ground-only directional and contact shadows precede all standing objects.
		for(const auto& h:houses)CastShadow(h.p,h.w*.7f,4.2f);
		for(const auto& tree:trees)CastShadow(tree.p,1.3f*tree.size,4.5f*tree.size);
		for(const auto& person:people)CastShadow(person.p,.5f,1.5f);
		for(const auto& animal:animals)CastShadow(animal.p,animal.kind==1?.4f:.8f,1.2f);
		CastShadow(player,.55f,1.7f);
		struct DrawItem{float depth;int type,index;};std::vector<DrawItem> items;
		for(int i=0;i<HouseCount;++i)if(Visible(houses[i].p))items.push_back({houses[i].p.x+houses[i].p.y,0,i});
		for(int i=0;i<(int)trees.size();++i)if(Visible(trees[i].p))items.push_back({trees[i].p.x+trees[i].p.y,1,i});
		for(int i=0;i<PersonCount;++i)if(Visible(people[i].p))items.push_back({people[i].p.x+people[i].p.y,2,i});
		items.push_back({player.x+player.y,3,0});
		items.push_back({rune.x+rune.y,4,0});items.push_back({fragment.x+fragment.y,4,1});items.push_back({shrine.x+shrine.y,4,2});
		for(int i=0;i<(int)animals.size();++i)if(Visible(animals[i].p))items.push_back({animals[i].p.x+animals[i].p.y,5,i});
		for(int i=0;i<4;++i)if(Visible(fires[i]))items.push_back({fires[i].x+fires[i].y,6,i});
		for(int i=0;i<5;++i)if(Visible(ruins[i]))items.push_back({ruins[i].x+ruins[i].y,7,i});
		std::stable_sort(items.begin(),items.end(),[](const DrawItem& a,const DrawItem& b){return a.depth<b.depth;});
		for(const auto& item:items)
		{
			switch(item.type)
			{
			case 0:DrawHouse(houses[item.index]);break;case 1:DrawTree(trees[item.index]);break;
			case 2:DrawPerson(people[item.index].p,people[item.index].coat,false,item.index);break;
			case 3:DrawPerson(player,{.25f,.47f,.51f,1},true,0);break;case 4:DrawRelic(item.index);break;
			case 5:DrawAnimal(animals[item.index]);break;case 6:DrawFire(fires[item.index]);break;
			case 7:Box(ruins[item.index],1.2f,1.2f,2.3f+item.index*.4f,{.49f,.51f,.40f,1},Material::Stone);break;
			}
		}
		for(int i=0;i<40;++i)
		{
			Point s=Project({10+(float)((i*7)%MapWidth),10+(float)((i*11)%(MapHeight-10))},1.8f+std::sin(elapsed*.5f+i));
			renderer->SoftEllipse(s.x,s.y,5,5,{1.6f,1.25f,.50f,.55f});
		}
		if(stage!=Complete)
		{
			Point target=Project(Target(),3.1f);
			if(target.x>30&&target.x<width-30&&target.y>150&&target.y<height-100)
			{
				target.y+=std::sin(elapsed*2)*4;
				renderer->Polygon({{target.x,target.y+8},{target.x-7,target.y-4},{target.x+7,target.y-4}},Gold);
			}
		}
	}
	void MiniMap(bool expanded)
	{
		float scale=expanded?std::min((width-160.f)/MapWidth,(height-200.f)/MapHeight):1.1f;
		float left=expanded?(width-MapWidth*scale)*.5f:width-200.f;
		float top=expanded?(height-MapHeight*scale)*.5f:155;
		renderer->Rect(left-12,top-32,MapWidth*scale+24,MapHeight*scale+52,Panel);
		renderer->Rect(left,top,MapWidth*scale,MapHeight*scale,{.18f,.27f,.22f,1});
		auto road=[&](const Point* points,int count)
		{
			for(int i=1;i<count;++i)
			{
				Point a=points[i-1],b=points[i];float d=Distance(a,b);
				for(float t=0;t<d;t+=.7f)renderer->Ellipse(left+(a.x+(b.x-a.x)*t/d)*scale,top+(a.y+(b.y-a.y)*t/d)*scale,scale*.65f,scale*.65f,{.58f,.53f,.35f,1});
			}
		};
		road(route,8);road(outerRoute,6);road(ridgeRoute,3);
		renderer->Ellipse(left+57*scale,top+28*scale,12*scale,10*scale,{.15f,.52f,.55f,1});
		for(const auto& h:houses)renderer->Rect(left+h.p.x*scale-3,top+h.p.y*scale-3,6,6,{.8f,.48f,.3f,1});
		if(expanded)
		{
			for(const auto& person:people)renderer->Ellipse(left+person.p.x*scale,top+person.p.y*scale,2,2,Teal);
			for(const auto& a:animals)renderer->Ellipse(left+a.p.x*scale,top+a.p.y*scale,2,2,{.65f,.72f,.42f,1});
		}
		if(stage!=Complete)renderer->Ellipse(left+Target().x*scale,top+Target().y*scale,5,5,Gold);
		renderer->Ellipse(left+player.x*scale,top+player.y*scale,4,4,Cream);
		renderer->Text(left,top-26,expanded?L"갈대여울 · 노란 점: 목표 / 흰 점: 나":L"여행 지도  [M]",Cream);
		if(expanded)
		{
			renderer->Text(left+5*scale,top+6*scale,L"속삭임의 숲",Cream);
			renderer->Text(left+49*scale,top+27*scale,L"거울 호수",Cream);
			renderer->Text(left+15*scale,top+59*scale,L"갈대여울 마을",Cream);
			renderer->Text(left+62*scale,top+89*scale,L"동쪽 농가",Cream);
			renderer->Text(left+104*scale,top+114*scale,L"옛 성터",Cream);
		}
	}
	void WrappedText(float x,float y,const std::wstring& text)
	{
		// Korean glyphs are roughly 18px; 46 characters fit the minimum window.
		for(size_t start=0;start<text.size();start+=46)
			renderer->Text(x,y+26*(float)(start/46),text.substr(start,46),Cream);
	}
	void Hud()
	{
		renderer->Rect(22,20,590,116,Panel);
		renderer->Rect(22,20,4,116,Gold);
		renderer->Text(42,31,L"갈대여울  /  지도 밖의 첫 걸음",Gold);
		renderer->Text(42,62,Objective(),Cream);
		int seconds=(int)(stage==Complete?completedAt:elapsed);
		std::wstring time=L"첫 여정  ·  "+std::to_wstring(seconds/60)+L"분 "+std::to_wstring(seconds%60)+L"초  /  목표 약 5분";
		renderer->Text(42,94,time,{.61f,.70f,.64f,1});
		MiniMap(false);
		renderer->Text(width-210.f,116,postprocess?L"[P] 후처리 켜짐":L"[P] 후처리 꺼짐",Teal);
		renderer->Rect(22,height-54.f,width-44.f,34,Panel);
		renderer->Text(38,height-49.f,L"WASD 이동   E 대화·조사·다음   M 지도   P 후처리 비교   Esc 일시정지",Cream);
		int nearbyId=Nearby();
		if(nearbyId>=0 && dialogue.empty() && !choosing && !paused && !showMap)
		{
			std::wstring name=nearbyId<PersonCount?people[nearbyId].name:nearbyId==PersonCount?L"오래된 표석":nearbyId==PersonCount+1?L"지도 조각":L"호숫가의 제단";
			renderer->Rect(width*.5f-180,height-105.f,360,38,Panel);
			renderer->Text(width*.5f-160,height-99.f,L"[E] "+name,Gold);
		}
		if(choice)
		{
			renderer->Rect(22,147,430,38,Panel);
			renderer->Text(38,153,choice==1?L"선택 이력 · 호수의 목소리에 응답함":L"선택 이력 · 호수의 연결을 잠재움",Teal);
		}
		if(!dialogue.empty())
		{
			renderer->Rect(40,height-247.f,width-80.f,180,Panel);
			renderer->Text(64,height-233.f,speaker,Gold);
			WrappedText(64,height-198.f,dialogue[page]);
			renderer->Text(64,height-103.f,L"[E / Space] 다음  "+std::to_wstring(page+1)+L" / "+std::to_wstring(dialogue.size()),Teal);
		}
		if(choosing)
		{
			renderer->Rect(40,height-250.f,width-80.f,185,Panel);
			renderer->Text(64,height-235.f,L"낯선 존재에게 어떻게 답할까요?",Gold);
			renderer->Text(64,height-196.f,L"[1] 종을 울려 응답한다 — 호수에 빛과 목소리가 돌아옵니다.",Cream);
			renderer->Text(64,height-160.f,L"[2] 종을 감싸 잠재운다 — 호수는 조용해집니다.",Cream);
			renderer->Text(64,height-115.f,L"선택 후에는 마을로 돌아가 발견을 전하세요.",Teal);
		}
		if(showMap) MiniMap(true);
		if(paused)
		{
			renderer->Rect(0,0,(float)width,(float)height,{.02f,.04f,.05f,.75f});
			renderer->Text(width*.5f-160,height*.5f-50,L"여행을 잠시 멈췄습니다",Gold);
			renderer->Text(width*.5f-160,height*.5f,L"Esc 계속   R 처음부터   Q 종료",Cream);
		}
		renderer->Flush();
	}
	void RenderScene()
	{
		renderer->Begin(width,height,elapsed,Project({0,0}));
		World(); renderer->FinishWorld(postprocess); Hud(); glutSwapBuffers();
	}
	void Reset()
	{
		player={18,50};camera=player;playerFacing=1;stage=Elder;choice=0;elapsed=0;completedAt=0;
		paused=false;showMap=false;choosing=false;dialogue.clear();afterDialogue={};
		animals.clear();
		for(int i=0;i<(int)(sizeof(animalHomes)/sizeof(animalHomes[0]));++i)
			animals.push_back({animalHomes[i],animalHomes[i],i%3,i*1.73f,0,0,false});
		std::fill(std::begin(keys),std::end(keys),false);
		Talk(L"지도 밖의 첫 걸음",{L"작은 마을 갈대여울. 숲 너머 호수에서 낯선 소리가 들려온다. 오늘은 그 목소리의 흔적을 찾아 떠나 보자.",
			L"WASD로 화면 방향에 맞춰 이동합니다. 주민이나 단서 가까이에서 E를 누르세요. 먼저 광장의 촌장 엘린을 찾아보세요."});
	}
	void KeyDown(unsigned char input,int,int)
	{
		unsigned char key=(unsigned char)std::towlower(input);
		if(key==27) {paused=!paused;std::fill(std::begin(keys),std::end(keys),false);return;}
		if(paused) {if(key=='r')Reset();if(key=='q')glutLeaveMainLoop();return;}
		if(key=='p') {postprocess=!postprocess;return;}
		if(key=='m' && dialogue.empty() && !choosing) {showMap=!showMap;return;}
		if(showMap) return;
		if(choosing) {if(key=='1'||key=='2')MakeChoice(key-'0');return;}
		if(!dialogue.empty())
		{
			if(key=='e'||key==' ')
			{
				if(++page==dialogue.size())
				{
					dialogue.clear(); auto done=afterDialogue;afterDialogue={};if(done)done();
				}
			}
			return;
		}
		if(key=='e') Interact(); else keys[key]=true;
	}
	void KeyUp(unsigned char key,int,int) {keys[(unsigned char)std::towlower(key)]=false;}
	void Tick(int)
	{
		float now=glutGet(GLUT_ELAPSED_TIME)*.001f;
		float realDelta=std::max(0.f,now-lastTick),dt=std::min(.05f,realDelta);lastTick=now;
		// Focus loss clears held keys so alt-tab cannot leave the player walking.
		if(GetForegroundWindow()!=WindowFromDC(wglGetCurrentDC()))
			std::fill(std::begin(keys),std::end(keys),false);
		walking=false;
		if(!paused)
		{
			elapsed+=realDelta;
			if(dialogue.empty()&&!choosing&&!showMap)
			{
				float sx=(keys['d']?1.f:0)-(keys['a']?1.f:0),sy=(keys['s']?1.f:0)-(keys['w']?1.f:0);
				// Invert the isometric projection: W/S follow screen vertical.
				float len=std::hypot(sx,sy);
				if(len>0)
				{
					Point previousPosition=player;
					playerFacing=std::abs(sx)>std::abs(sy)?(sx>0?0:2):(sy>0?1:3);
					// Normalize screen input before inverse projection (84 pixels/second).
					float vx=sx/len*84.f*dt,vy=sy/len*84.f*dt;
					float dx=(vx/23.f+vy/11.5f)*.5f,dy=(vy/11.5f-vx/23.f)*.5f;
					if(CanWalk({player.x+dx,player.y})) {player.x+=dx;}
					if(CanWalk({player.x,player.y+dy})) {player.y+=dy;}
					walking=Distance(previousPosition,player)>.0001f;
				}
			}
			if(dialogue.empty()&&!choosing&&!showMap)UpdateAnimals(dt);
			else for(auto& animal:animals)animal.speed=0;
			camera.x+=(player.x-camera.x)*std::min(1.f,dt*7);
			camera.y+=(player.y-camera.y)*std::min(1.f,dt*7);
		}
		glutPostRedisplay();glutTimerFunc(16,Tick,0);
	}
	void Resize(int w,int h)
	{
		width=std::max(1100,w);height=std::max(700,h);
		if(w<1100||h<700) glutReshapeWindow(width,height);
	}
	void Close()
	{
		delete renderer;renderer=nullptr;
	}
}

int main(int argc,char** argv)
{
	glutInit(&argc,argv);
	glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
	glutInitWindowSize(width,height);
	glutCreateWindow("Reedmere - The First Uncharted Step");
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	if(glewInit()!=GLEW_OK || !GLEW_VERSION_3_3) {MessageBoxW(nullptr,L"OpenGL 3.3 지원이 필요합니다.",L"렌더러 초기화 실패",MB_OK);return 1;}
	renderer=new Renderer(width,height);
	if(!renderer->IsInitialized()) {MessageBoxW(nullptr,L"셰이더 또는 글꼴 초기화에 실패했습니다. 콘솔을 확인하세요.",L"초기화 실패",MB_OK);Close();return 1;}
	// Keep paths, people, camps and animal starting points clear in the expanded world.
	for(int i=0;i<1100;++i)
	{
		Point p={3+(float)((i*37)%(MapWidth-6)),3+(float)((i*23)%(MapHeight-6))};
		if(InLake(p,2)||OnPath(p)||Distance(p,{20,49})<16||Distance(p,shrine)<4)continue;
		if(Distance(p,{67,79})<12||Distance(p,{132,107})<10)continue;
		bool clear=false;
		for(const auto& h:houses)if(Distance(p,h.p)<5)clear=true;
		for(const auto& person:people)if(Distance(p,person.p)<3)clear=true;
		for(Point home:animalHomes)if(Distance(p,home)<2.5f)clear=true;
		for(Point fire:fires)if(Distance(p,fire)<3)clear=true;
		if(clear)continue;
		trees.push_back({p,.8f+(i%5)*.1f,i%4==0?1:0});
	}
	Reset();lastTick=glutGet(GLUT_ELAPSED_TIME)*.001f;
	glutDisplayFunc(RenderScene);glutReshapeFunc(Resize);
	glutKeyboardFunc(KeyDown);glutKeyboardUpFunc(KeyUp);glutIgnoreKeyRepeat(1);
	glutCloseFunc(Close);glutTimerFunc(16,Tick,0);
	glutMainLoop();
	return 0;
}
