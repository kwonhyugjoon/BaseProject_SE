#ifdef _WIN32
#include <windows.h>
#endif
#include "../SimpleGame/FontRasterizer.h"
#include <map>
#include <functional>
#include <initializer_list>
#include <string>
#define private public
#include "../SimpleGame/Renderer.h"
#undef private
#define main TutorialApplicationMain
#include "../SimpleGame/SimpleGame.cpp"
#undef main
#include <cassert>
#include <cstdio>
#include <numeric>
#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
	std::vector<unsigned char> Pixels()
	{
		std::vector<unsigned char> pixels(width * height * 4);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		return pixels;
	}

#ifdef _WIN32
	void CompareLegacyText(Renderer& r)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glColor4f(1, 1, 1, 1);
		glRasterPos2f(40, 68);
		HDC dc = wglGetCurrentDC();
		HFONT font = CreateFontW(-18,
		                         0,
		                         0,
		                         0,
		                         FW_MEDIUM,
		                         FALSE,
		                         FALSE,
		                         FALSE,
		                         DEFAULT_CHARSET,
		                         OUT_DEFAULT_PRECIS,
		                         CLIP_DEFAULT_PRECIS,
		                         ANTIALIASED_QUALITY,
		                         DEFAULT_PITCH,
		                         L"Malgun Gothic");
		HGDIOBJ old = SelectObject(dc, font);
		for (wchar_t code : std::wstring(L"갈대여울 ABC123 gjpqy!? →"))
		{
			GLuint list = glGenLists(1);
			assert(wglUseFontBitmapsW(dc, code, 1, list));
			glCallList(list);
			glDeleteLists(list, 1);
		}
		SelectObject(dc, old);
		DeleteObject(font);
		auto before = Pixels();
		r.Begin(width, height, 0, {0, 0});
		r.FinishWorld(false);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		r.Text(40, 50, L"갈대여울 ABC123 gjpqy!? →", {1, 1, 1, 1});
		r.Flush();
		auto after = Pixels();
		size_t differences = 0;
		for (size_t i = 0; i < before.size(); ++i)
			differences += before[i] != after[i];
		printf("Legacy/new glyph channel differences: %zu / %zu\n", differences, before.size());
		assert(differences == 0);
	}
#endif
}

int main(int argc, char** argv)
{
	bool legacyComparison = argc < 2 || std::string(argv[1]) != "--core";
	assert(glfwInit());
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef _WIN32
	// Compatibility is used only for the old WGL-font comparison.
	glfwWindowHint(GLFW_OPENGL_PROFILE,
	               legacyComparison ? GLFW_OPENGL_COMPAT_PROFILE : GLFW_OPENGL_CORE_PROFILE);
#else
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
#endif
	gameWindow = glfwCreateWindow(width, height, "Hidden migration verification", nullptr, nullptr);
	assert(gameWindow);
	glfwMakeContextCurrent(gameWindow);
	glfwSwapInterval(0);
	glewExperimental = GL_TRUE;
	assert(glewInit() == GLEW_OK && GLEW_VERSION_3_3);
	glGetError();
	GLuint releasedFont = 0, releasedBuffer = 0;
	{
		Renderer r(width, height);
		assert(r.IsInitialized());
		renderer = &r;
		releasedFont = r.m_FontAtlas;
		releasedBuffer = r.m_Buffer;
#ifdef _WIN32
		if (legacyComparison)
			CompareLegacyText(r);
#endif
		BuildTutorialForest();
		BuildTutorialScene();
		Reset();
		auto drawVillage = [&]()
		{
			r.Begin(width, height, 0, Project({0, 0}));
			tutorialScene.DrawWorld(r);
			r.FinishWorld(true);
			tutorialScene.DrawUI(r);
			r.Flush();
		};
		for (int i = 0; i < 10; ++i)
			drawVillage();
		glFinish();
		size_t glyphCount = r.m_Glyphs.size(), vertexCapacity = r.m_Vertices.capacity();
		int atlasX = r.m_FontX, atlasY = r.m_FontY;
		GLuint atlasTexture = r.m_FontAtlas;
		auto start = FrameClock::now();
		for (int i = 0; i < 60; ++i)
			drawVillage();
		assert(r.m_Glyphs.size() == glyphCount && r.m_FontX == atlasX && r.m_FontY == atlasY);
		assert(r.m_FontAtlas == atlasTexture && r.m_Vertices.capacity() == vertexCapacity);
		printf("Stable cached glyphs: %zu; font atlas: %d bytes; vertex capacity: %zu\n",
		       glyphCount,
		       Renderer::FontAtlasSize * Renderer::FontAtlasSize,
		       vertexCapacity);
		glFinish();
		double batchMs =
		    std::chrono::duration<double, std::milli>(FrameClock::now() - start).count() / 60;
		assert(glGetError() == GL_NO_ERROR);
		auto pixels = Pixels();
		assert(std::accumulate(pixels.begin(), pixels.end(), uint64_t(0)) >
		       width * height * 255ull);
		printf("Hidden village render, CPU+GPU batch mean: %.3f ms\n", batchMs);
		firstLevel.Begin(42, false, WorldTravel::Region::DwarfCity);
		firstLevel.Draw(r, width, height, true, false);
		glFinish();
		assert(glGetError() == GL_NO_ERROR);
		renderer = nullptr;
	}
	assert(!glIsTexture(releasedFont) && !glIsBuffer(releasedBuffer));
	glfwDestroyWindow(gameWindow);
	gameWindow = nullptr;
	glfwTerminate();
	puts("PASS: hidden GLFW/OpenGL context, shaders, Korean text, village and dwarf city.");
}
