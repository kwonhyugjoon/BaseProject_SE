// Exercise the tutorial's internal data without opening its GLFW application.
#define main TutorialApplicationMain
#include "../SimpleGame/SimpleGame.cpp"
#undef main
#include <cassert>
#include <cstdio>

int main()
{
	assert(PersonCount == 38 && MapWidth == 192 && MapHeight == 176);
	BuildTutorialScene();
	Reset();
	MonitorSnapshot snapshot = CaptureMonitorState();
	assert(snapshot.npcs.size() == 43 && snapshot.villages.size() == 3);
	assert(snapshot.villages[0].population == 38 && snapshot.villages[0].mood.empty());
	assert(snapshot.villages[1].population == 3 && snapshot.npcs[38].state == "inactive_area");
	assert(!snapshot.huntingLoaded && snapshot.npcs[0].state == "waiting");
	dialogue.clear();
	assert(CaptureMonitorState().npcs[0].state == "standing");
	inFirstLevel = true;
	assert(CaptureMonitorState().npcs[0].state == "inactive_area");
	inFirstLevel = false;
	paused = true;
	assert(CaptureMonitorState().npcs[0].state == "paused");
	paused = false;
	for (Stage expected : {Elder, Scout, Rune, Fragment, Fisher})
		assert(questJournal.CompleteStep(expected));
	assert(questJournal.ChooseLake(2));
	assert(CaptureMonitorState().villages[0].lakeChoice == "close");
	questJournal.Reset();
	dialogue.clear();
	for (int i = 3; i < PersonCount; ++i)
	{
		player = {people[i].home.x + 10, people[i].home.y + 10};
		bool moved = false;
		for (int step = 0; step < 1800; ++step)
		{
			elapsed += 1.f / 120;
			UpdatePerson(i, 1.f / 120);
			assert(CanWalk(people[i].p));
			moved = moved || people[i].moving;
		}
		assert(moved);
	}
	for (int i = 0; i < 3; ++i)
		assert(Distance(people[i].p, people[i].home) == 0);
	// All twenty new dialogue indices must stay inside their own dialogue table.
	for (int i = 18; i < PersonCount; ++i)
	{
		player = people[i].p;
		Interact();
		assert(!dialogue.empty());
		dialogue.clear();
	}
	Reset();
	for (const auto& person : people)
		assert(Distance(person.p, person.home) == 0 && !person.moving);
	puts("38 NPCs, 35 moving routines, 20 dialogue indices, quest positions and reset passed.");
}
