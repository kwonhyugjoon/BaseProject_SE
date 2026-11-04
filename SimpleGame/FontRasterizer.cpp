#include "stdafx.h"
#include "FontRasterizer.h"
#include "PlatformPaths.h"
#include <cmath>
#include <fstream>
#include <iterator>
#define STB_TRUETYPE_IMPLEMENTATION
#include "../ThirdParty/stb/stb_truetype.h"
#ifdef _WIN32
#include <windows.h>
#endif

struct FontRasterizer::Impl
{
	std::vector<unsigned char> data;
	stbtt_fontinfo info = {};
	bool loaded = false;
#ifdef _WIN32
	HDC dc = nullptr;
	HFONT font = nullptr;
	HGDIOBJ previous = nullptr;

	~Impl()
	{
		if (previous)
			SelectObject(dc, previous);
		if (font)
			DeleteObject(font);
		if (dc)
			DeleteDC(dc);
	}
#endif
};

FontRasterizer::FontRasterizer() : m_Impl(new Impl)
{
}

FontRasterizer::~FontRasterizer() = default;

bool FontRasterizer::UsesSystemFont() const
{
#ifdef _WIN32
	return m_Impl->font != nullptr;
#else
	return false;
#endif
}

bool FontRasterizer::Initialize(bool systemFont)
{
	std::ifstream file(ResourcePath("Fonts/NanumGothic-Regular.ttf"), std::ios::binary);
	m_Impl->data.assign(std::istreambuf_iterator<char>(file), {});
	if (!m_Impl->data.empty())
		m_Impl->loaded = stbtt_InitFont(&m_Impl->info, m_Impl->data.data(), 0) != 0;
#ifdef _WIN32
	if (systemFont && !m_Impl->font)
	{
		m_Impl->dc = CreateCompatibleDC(nullptr);
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
		if (m_Impl->dc && font)
		{
			HGDIOBJ previous = SelectObject(m_Impl->dc, font);
			wchar_t face[64] = {};
			GetTextFaceW(m_Impl->dc, 64, face);
			if (_wcsicmp(face, L"Malgun Gothic") == 0 || wcscmp(face, L"맑은 고딕") == 0)
			{
				m_Impl->font = font;
				m_Impl->previous = previous;
			}
			else
			{
				SelectObject(m_Impl->dc, previous);
				DeleteObject(font);
			}
		}
		else if (font)
			DeleteObject(font);
	}
#else
	(void)systemFont;
#endif
	return m_Impl->loaded || UsesSystemFont();
}

GlyphBitmap FontRasterizer::Rasterize(uint32_t codepoint) const
{
	GlyphBitmap glyph;
#ifdef _WIN32
	if (m_Impl->font && codepoint <= 0xffff)
	{
		wchar_t character = static_cast<wchar_t>(codepoint);
		WORD index = 0xffff;
		GetGlyphIndicesW(m_Impl->dc, &character, 1, &index, GGI_MARK_NONEXISTING_GLYPHS);
		if (index != 0xffff)
		{
			MAT2 transform = {};
			transform.eM11.value = transform.eM22.value = 1;
			GLYPHMETRICS metrics = {};
			DWORD bytes = GetGlyphOutlineW(
			    m_Impl->dc, codepoint, GGO_BITMAP, &metrics, 0, nullptr, &transform);
			if (bytes != GDI_ERROR)
			{
				glyph.width = static_cast<int>(metrics.gmBlackBoxX);
				glyph.height = static_cast<int>(metrics.gmBlackBoxY);
				glyph.left = metrics.gmptGlyphOrigin.x;
				glyph.top = -metrics.gmptGlyphOrigin.y;
				glyph.advance = metrics.gmCellIncX;
				glyph.pixels.resize(glyph.width * glyph.height);
				if (bytes)
				{
					std::vector<unsigned char> bitmap(bytes);
					GetGlyphOutlineW(m_Impl->dc,
					                 codepoint,
					                 GGO_BITMAP,
					                 &metrics,
					                 bytes,
					                 bitmap.data(),
					                 &transform);
					int stride = ((glyph.width + 31) / 32) * 4;
					for (int y = 0; y < glyph.height; ++y)
						for (int x = 0; x < glyph.width; ++x)
							glyph.pixels[y * glyph.width + x] =
							    (bitmap[y * stride + x / 8] & (0x80 >> (x % 8))) ? 255 : 0;
				}
				return glyph;
			}
		}
	}
#endif
	if (!m_Impl->loaded)
		return glyph;
	int index = stbtt_FindGlyphIndex(&m_Impl->info, static_cast<int>(codepoint));
	if (!index)
		index = stbtt_FindGlyphIndex(&m_Impl->info, '?');
	float scale = stbtt_ScaleForMappingEmToPixels(&m_Impl->info, 18);
	int advance = 0, bearing = 0, x1 = 0, y1 = 0;
	stbtt_GetGlyphHMetrics(&m_Impl->info, index, &advance, &bearing);
	stbtt_GetGlyphBitmapBox(&m_Impl->info, index, scale, scale, &glyph.left, &glyph.top, &x1, &y1);
	glyph.width = x1 - glyph.left;
	glyph.height = y1 - glyph.top;
	glyph.advance = static_cast<int>(std::round(advance * scale));
	glyph.pixels.resize(glyph.width * glyph.height);
	if (!glyph.pixels.empty())
		stbtt_MakeGlyphBitmap(&m_Impl->info,
		                      glyph.pixels.data(),
		                      glyph.width,
		                      glyph.height,
		                      glyph.width,
		                      scale,
		                      scale,
		                      index);
	return glyph;
}
