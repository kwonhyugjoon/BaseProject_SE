// Verify quest recording and journal input through the existing game handlers.
#define main TutorialApplicationMain
#include "../SimpleGame/SimpleGame.cpp"
#undef main
#include <cassert>
#include <cstdio>

namespace
{
	void ReadDialogue()
	{
		while (!dialogue.empty())
			KeyDown(' ', 0, 0);
	}

	void Speak(Point position)
	{
		playerActor.SetWorldPosition(position);
		KeyDown('e', 0, 0);
		assert(!dialogue.empty());
		ReadDialogue();
	}

	void SurveyCurrentRegion()
	{
		Actor* marker = firstLevel.Scene().World().Find("SurveyStone");
		Actor* traveler = firstLevel.Scene().World().Find("Player");
		assert(marker && traveler);
		traveler->SetWorldPosition(marker->WorldPosition());
		KeyDown('e', 0, 0);
		assert(questJournal.Investigated(firstLevel.CurrentRegion()));
		auto clues = questJournal.Lines(1, firstLevel.CurrentRegion());
		KeyDown('e', 0, 0);
		assert(questJournal.Lines(1, firstLevel.CurrentRegion()) == clues);
	}

	void CheckRoute(WorldTravel::Region start)
	{
		auto route = questJournal.RouteToNext(start);
		assert(!route.empty() && route.front() == start);
		assert(route.back() == questJournal.NextDestination());
		for (size_t i = 1; i < route.size(); ++i)
		{
			bool linked = false;
			for (const auto& exit : WorldTravel::Exits(route[i - 1]))
				linked = linked || exit.destination == route[i];
			assert(linked);
		}
	}
}

int main()
{
	using Region = WorldTravel::Region;
	BuildTutorialScene();
	for (int selection : {1, 2})
	{
		Reset();
		KeyDown('j', 0, 0);
		assert(!questJournal.IsOpen()); // Intro cannot be skipped by opening the journal.
		ReadDialogue();
		assert(!questJournal.ChooseLake(selection));
		assert(!questJournal.CompleteStep(Rune));
		auto before = questJournal.Lines(1, Region::Village);
		Speak(rune); // Visiting a later clue must not complete an earlier step.
		assert(stage == Elder && questJournal.Lines(1, Region::Village) == before);

		KeyDown('w', 0, 0);
		KeyDown('j', 0, 0);
		assert(questJournal.IsOpen() && !keys['w']);
		KeyDown('e', 0, 0);
		KeyDown('3', 0, 0);
		assert(questJournal.IsOpen() && stage == Elder && !inFirstLevel);
		KeyDown(27, 0, 0);
		assert(!questJournal.IsOpen() && !paused);

		Speak(people[0].p);
		assert(stage == Scout);
		Speak(people[1].p);
		Speak(rune);
		Speak(fragment);
		Speak(people[2].p);
		Speak(shrine);
		assert(stage == Shrine && choosing);
		KeyDown('j', 0, 0);
		assert(!questJournal.IsOpen());
		MakeChoice(3);
		assert(choice == 0 && stage == Shrine && choosing);
		KeyDown(static_cast<unsigned char>('0' + selection), 0, 0);
		ReadDialogue();
		assert(choice == selection && stage == Return);
		assert(!questJournal.ChooseLake(3 - selection));
		Speak(people[0].p);
		assert(stage == Complete && questJournal.NextDestination() == Region::Basin);
		for (int i = 1; i < static_cast<int>(Region::Count); ++i)
			CheckRoute(static_cast<Region>(i));

		EnterRegion(Region::Passage);
		Actor* traveler = firstLevel.Scene().World().Find("Player");
		Point original = traveler->WorldPosition();
		firstLevel.Key('m');
		KeyDown('j', 0, 0);
		assert(!questJournal.IsOpen());
		firstLevel.Key('m');
		KeyDown('j', 0, 0);
		bool input[256] = {};
		input['d'] = input[' '] = true;
		assert(questJournal.IsOpen() && !firstLevel.Update(.25f, input));
		assert(Distance(traveler->WorldPosition(), original) == 0 && !firstLevel.Key('e'));
		KeyDown('j', 0, 0);
		assert(!questJournal.IsOpen());
		EnterRegion(Region::Basin, Region::Passage);
		SurveyCurrentRegion();
		assert(questJournal.NextDestination() == Region::StoneRidge);
		auto routes = questJournal.Lines(2, Region::Basin);
		questJournal.RecordTravel(Region::Basin, Region::Passage);
		assert(questJournal.Lines(2, Region::Basin) == routes);
		questJournal.RecordTravel(Region::Village, Region::MistShore);
		assert(
		    !questJournal.Visited(Region::MistShore)); // Nonexistent connections are not recorded.

		EnterRegion(Region::EchoHall, Region::Basin);
		assert(!questJournal.Investigate(Region::EchoHall));
		EnterRegion(Region::MistShore, Region::EchoHall);
		SurveyCurrentRegion(); // Explore out of order, then finish the missing region.
		assert(questJournal.NextDestination() == Region::StoneRidge);
		EnterRegion(Region::EchoHall, Region::MistShore);
		EnterRegion(Region::StoneRidge, Region::EchoHall);
		SurveyCurrentRegion();
		assert(questJournal.NextDestination() == Region::StoneRidge);
		auto clues = questJournal.Lines(1, Region::StoneRidge);
		firstLevel.Begin(42, true, Region::StoneRidge);
		assert(questJournal.Lines(1, Region::StoneRidge) == clues && choice == selection);
		assert(questJournal.RouteToNext(Region::StoneRidge).size() == 1);

		// Use the real exit handlers to return home without changing history.
		traveler->SetWorldPosition(WorldTravel::NearGate);
		KeyDown('e', 0, 0); // Ridge -> hall.
		traveler->SetWorldPosition(WorldTravel::NearGate);
		KeyDown('e', 0, 0); // Hall -> basin.
		traveler->SetWorldPosition(WorldTravel::NearGate);
		KeyDown('e', 0, 0); // Basin -> passage.
		traveler->SetWorldPosition(WorldTravel::NearGate);
		KeyDown('e', 0, 0); // Passage -> village.
		assert(!inFirstLevel && stage == Complete && choice == selection);
		assert(questJournal.Lines(1, Region::Village) == clues);
	}

	// Opening and finishing a regional dialogue must clear held movement/attack input.
	Reset();
	dialogue.clear();
	for (Stage expected : {Elder, Scout, Rune, Fragment, Fisher})
		assert(questJournal.CompleteStep(expected));
	assert(questJournal.ChooseLake(1) && questJournal.CompleteStep(Return));
	questJournal.Visit(Region::StoneRidge);
	assert(questJournal.Investigate(Region::StoneRidge));
	EnterRegion(Region::StoneRidge);
	Actor* contact = firstLevel.Scene().World().Find("Dwarf/Torvik");
	firstLevel.Scene().World().Find("Player")->SetWorldPosition(contact->WorldPosition());
	keys['w'] = keys[' '] = true;
	KeyDown('e', 0, 0);
	assert(firstLevel.IsStoryOpen() && !keys['w'] && !keys[' ']);
	KeyDown('j', 0, 0);
	assert(!questJournal.IsOpen());
	for (int i = 0; i < 3; ++i)
		KeyDown(' ', 0, 0);
	assert(!firstLevel.IsStoryOpen() && !keys[' '] && !keys['w']);
	assert(questJournal.DwarfProgress() == QuestJournal::DwarfStage::Record);
	inFirstLevel = false;
	Reset();
	assert(stage == Elder && choice == 0 && !questJournal.IsOpen());
	assert(!questJournal.Investigated(WorldTravel::Region::Basin));
	assert(!questJournal.Visited(WorldTravel::Region::EchoHall));
	assert(questJournal.Lines(2, WorldTravel::Region::Village).size() == 3);
	puts(
	    "PASS: both choices, ordered clues, routes, out-of-order surveys, journal input, reset and retained history.");
}
