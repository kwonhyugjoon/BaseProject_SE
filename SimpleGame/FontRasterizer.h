#pragma once
#include <cstdint>
#include <memory>
#include <vector>

struct GlyphBitmap
{
	int width = 0, height = 0;
	int left = 0, top = 0, advance = 0;
	std::vector<unsigned char> pixels;
};

class FontRasterizer
{
  public:
	FontRasterizer();
	~FontRasterizer();
	bool Initialize(bool systemFont = true);
	bool UsesSystemFont() const;
	GlyphBitmap Rasterize(uint32_t codepoint) const;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_Impl;
};
