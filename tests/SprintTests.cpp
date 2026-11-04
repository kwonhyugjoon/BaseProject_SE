#define main TutorialApplicationMain
#include "../SimpleGame/SimpleGame.cpp"
#undef main
#include <cassert>
#include <cstdio>

namespace
{
	void Movement(bool* input, Point axes, bool sprint)
	{
		std::fill(input, input + 256, false);
		input['d'] = axes.x > 0;
		input['a'] = axes.x < 0;
		input['s'] = axes.y > 0;
		input['w'] = axes.y < 0;
		input[GameInput::Sprint] = sprint;
	}

	float ScreenDistance(Point before, Point after)
	{
		float x = after.x - before.x, y = after.y - before.y;
		return std::hypot((x - y) * 23, (x + y) * 11.5f);
	}
}

int main()
{
	BuildTutorialScene();
	Reset();
	dialogue.clear();
	QuestJournal journal;
	FirstLevel level(journal);
	level.Begin(42);
	Actor* traveler = level.Scene().World().Find("Player");
	constexpr float dt = 1.f / 120;
	const Point directions[] = {
	    {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
	bool input[256] = {};
	for (bool sprint : {false, true})
		for (Point direction : directions)
		{
			Point start = VillagePoint({20, 49});
			playerActor.SetWorldPosition(start);
			Movement(keys, direction, sprint);
			UpdatePlayer(dt);
			assert(std::abs(ScreenDistance(start, player) - (sprint ? 168.f : 84.f) * dt) < .002f);
			traveler->SetWorldPosition(WorldTravel::NearGate);
			Movement(input, direction, sprint);
			level.Update(dt, input);
			assert(std::abs(ScreenDistance(WorldTravel::NearGate, traveler->WorldPosition()) -
			                (sprint ? 168.f : 84.f) * dt) < .002f);
		}
	// Releasing Shift restores walking speed while the movement key remains held.
	Point start = player;
	keys[GameInput::Sprint] = false;
	UpdatePlayer(dt);
	assert(std::abs(ScreenDistance(start, player) - 84.f * dt) < .002f);
	Movement(keys, {0, 0}, true);
	start = player;
	UpdatePlayer(dt);
	assert(ScreenDistance(start, player) == 0);

	// Running into a house must not cross its collision boundary.
	const House& house = houses[0];
	start = {house.p.x - house.w * .5f - .42f, house.p.y};
	playerActor.SetWorldPosition(start);
	Movement(keys, {1, 1}, true);
	UpdatePlayer(dt);
	assert(player.x == start.x && CanWalk(player));

	// A prop immediately beside a known enemy spawn provides a real map-wall case.
	bool checkedWall = false;
	Actor* terrain = level.Scene().World().Find("Terrain");
	Actor* enemies = level.Scene().World().Find("Enemies");
	for (const auto& chunk : terrain->Children())
	{
		for (const auto& prop : chunk->Children())
		{
			if (prop->Name().find("Prop/") != 0)
				continue;
			Point wall = prop->WorldPosition();
			bool leftIsGround = false;
			for (const auto& enemy : enemies->Children())
			{
				Point p = enemy->WorldPosition();
				leftIsGround = leftIsGround || (p.x == wall.x - 2 && p.y == wall.y);
			}
			if (!leftIsGround)
				continue;
			start = {wall.x - 1.26f, wall.y};
			traveler->SetWorldPosition(start);
			Movement(input, {1, 1}, true);
			level.Update(dt, input);
			assert(traveler->WorldPosition().x == start.x);
			checkedWall = true;
			break;
		}
		if (checkedWall)
			break;
	}
	assert(checkedWall);
	journal.Open();
	start = traveler->WorldPosition();
	assert(!level.Update(.25f, input));
	assert(ScreenDistance(start, traveler->WorldPosition()) == 0);
	Movement(keys, {1, 0}, true);
	KeyDown('j', 0, 0);
	assert(questJournal.IsOpen() && !keys['d'] && !keys[GameInput::Sprint]);
	KeyDown('d', 0, 0);
	assert(!keys['d']);
	KeyDown(27, 0, 0);
	assert(!questJournal.IsOpen() && !paused);
	for (Point viewport : {Point{1280, 800}, Point{1920, 1080}, Point{3840, 2160}})
		for (Point extent : {Point{static_cast<float>(MapWidth), static_cast<float>(MapHeight)},
		                     Point{WorldTravel::Columns * 2.f, WorldTravel::Rows * 2.f}})
			for (Point cameraPosition : {Point{4, 4},
			                             Point{extent.x - 4, 4},
			                             Point{4, extent.y - 4},
			                             Point{extent.x - 4, extent.y - 4}})
				for (Point corner :
				     {Point{0, 0}, Point{viewport.x, 0}, Point{0, viewport.y}, viewport})
				{
					float sx = corner.x - viewport.x * .5f, sy = corner.y - viewport.y * .53f;
					Point ground = {cameraPosition.x + (sx / 23 + sy / 11.5f) * .5f,
					                cameraPosition.y + (sy / 11.5f - sx / 23) * .5f};
					assert(ground.x >= -WorldTravel::VisualMargin &&
					       ground.y >= -WorldTravel::VisualMargin);
					assert(ground.x <= extent.x + WorldTravel::VisualMargin &&
					       ground.y <= extent.y + WorldTravel::VisualMargin);
				}

	puts(
	    "PASS: eight-direction walk/sprint speeds, Shift release, collisions and journal input blocking.");
}
