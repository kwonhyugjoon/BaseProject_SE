#define main TutorialApplicationMain
#include "../SimpleGame/SimpleGame.cpp"
#undef main
#include <cassert>
#include <cstdio>

int main()
{
	BuildTutorialScene();
	Reset();
	dialogue.clear();
	WindowKey(nullptr, GLFW_KEY_W, 0, GLFW_PRESS, 0);
	assert(keys['w']);
	WindowKey(nullptr, GLFW_KEY_W, 0, GLFW_RELEASE, 0);
	assert(!keys['w']);
	WindowKey(nullptr, GLFW_KEY_W, 0, GLFW_REPEAT, 0);
	assert(!keys['w']);
	keys['d'] = keys[GameInput::Sprint] = true;
	FocusChanged(nullptr, GLFW_FALSE);
	assert(!keys['d'] && !keys[GameInput::Sprint]);
	WindowKey(nullptr, GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
	assert(paused);
	WindowKey(nullptr, GLFW_KEY_ESCAPE, 0, GLFW_REPEAT, 0);
	assert(paused);
	WindowKey(nullptr, GLFW_KEY_ESCAPE, 0, GLFW_RELEASE, 0);
	WindowKey(nullptr, GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
	assert(!paused);
	WindowKey(nullptr, GLFW_KEY_J, 0, GLFW_PRESS, 0);
	assert(questJournal.IsOpen());
	WindowKey(nullptr, GLFW_KEY_2, 0, GLFW_PRESS, 0);
	assert(questJournal.IsOpen() && choice == 0);
	WindowKey(nullptr, GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
	assert(!questJournal.IsOpen() && !paused);
	Resize(1920, 1080);
	assert(width == 1920 && height == 1080);
	Resize(0, 0);
	assert(width == 1920 && height == 1080);
	puts(
	    "PASS: GLFW key callbacks, repeat rejection, focus clearing, overlays and framebuffer resize.");
}
