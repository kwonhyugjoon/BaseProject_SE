#include "../SimpleGame/FontRasterizer.h"
#include "../SimpleGame/MonitorState.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
	FontRasterizer portable;
	assert(portable.Initialize(false) && !portable.UsesSystemFont());
	for (uint32_t code : std::vector<uint32_t>{'A', '1', 0xac00, 0xd7a3, 0x2192})
	{
		auto glyph = portable.Rasterize(code);
		assert(glyph.advance > 0 && glyph.width > 0 && glyph.height > 0);
		assert(glyph.pixels.size() == static_cast<size_t>(glyph.width * glyph.height));
		assert(std::any_of(glyph.pixels.begin(),
		                   glyph.pixels.end(),
		                   [](unsigned char p)
		                   {
			                   return p != 0;
		                   }));
	}
	assert(portable.Rasterize(' ').advance > 0);
	FontRasterizer system;
	assert(system.Initialize());
#ifdef _WIN32
	if (system.UsesSystemFont())
	{
		HDC dc = CreateCompatibleDC(nullptr);
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
		for (wchar_t code : std::wstring(L"갈대여울 ABC123 →"))
		{
			SIZE extent = {};
			GetTextExtentPoint32W(dc, &code, 1, &extent);
			auto glyph = system.Rasterize(code);
			assert(glyph.advance == extent.cx);
			assert(std::all_of(glyph.pixels.begin(),
			                   glyph.pixels.end(),
			                   [](unsigned char p)
			                   {
				                   return p == 0 || p == 255;
			                   }));
		}
		SelectObject(dc, old);
		DeleteObject(font);
		DeleteDC(dc);
	}
#endif
	assert(MonitorUtf8(L"가 🌿") == u8"가 🌿");
	puts("PASS: bundled Korean font, native text advances/coverage and UTF-8.");
}
