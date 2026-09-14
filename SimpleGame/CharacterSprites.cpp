#include "stdafx.h"
#include "CharacterSprites.h"
#include "AssetCache.h"
#include <cmath>

std::vector<unsigned char> CreateCharacterAtlas()
{
	const int width = SpriteWidth * SpriteFrames;
	const size_t byteCount = width * SpriteHeight * SpriteDirections * 4;
	std::vector<unsigned char> pixels;
	if (LoadAssetCache(L"character-v1.rgba", 1, byteCount, pixels) && pixels.size() == byteCount)
		return pixels;
	pixels.assign(byteCount, 0);

	for (int direction = 0; direction < SpriteDirections; ++direction)
		for (int frame = 0; frame < SpriteFrames; ++frame)
		{
			const int stride =
			    frame == 8 ? 0 : (int)std::round(std::sin(frame * 6.2831853f / 8) * 4);
			const int bob = frame == 8 ? 0 : (frame % 4 == 1 ? -1 : 0);
			auto pixel = [&](int x, int y, int r, int g, int b)
			{
				if (x < 0 || x >= SpriteWidth || y < 0 || y >= SpriteHeight)
					return;

				if (direction == 2)
					x = SpriteWidth - 1 - x;
				size_t index =
				    ((direction * SpriteHeight + y) * width + frame * SpriteWidth + x) * 4;
				pixels[index] = (unsigned char)r;
				pixels[index + 1] = (unsigned char)g;
				pixels[index + 2] = (unsigned char)b;
				pixels[index + 3] = 255;
			};
			auto rect = [&](int x, int y, int w, int h, int r, int g, int b)
			{
				for (int py = y; py < y + h; ++py)
					for (int px = x; px < x + w; ++px)
						pixel(px, py, r, g, b);
			};
			auto oval = [&](int x, int y, int rx, int ry, int r, int g, int b)
			{
				for (int py = -ry; py <= ry; ++py)
					for (int px = -rx; px <= rx; ++px)
						if ((float)(px * px) / (rx * rx) + (float)(py * py) / (ry * ry) <= 1)
							pixel(x + px, y + py, r, g, b);
			};

			// Boots and trousers: alternating feet, articulated hands, tunic and belt.
			rect(16, 43, 6, 13 + stride, 41, 46, 42);
			rect(26, 43, 6, 13 - stride, 38, 42, 39);
			rect(14, 54 + stride, 9, 5, 46, 32, 25);
			rect(25, 54 - stride, 9, 5, 46, 32, 25);
			rect(14, 54 + stride, 9, 1, 108, 78, 48);
			rect(25, 54 - stride, 9, 1, 108, 78, 48);
			oval(13, 36 + bob - stride, 4, 10, 27, 35, 31);
			oval(34, 36 + bob + stride, 4, 10, 27, 35, 31);
			oval(13, 43 + bob - stride, 3, 4, 195, 145, 100);
			oval(34, 43 + bob + stride, 3, 4, 195, 145, 100);
			oval(24, 34 + bob, 11, 15, 25, 34, 30);

			// Blue is a clothing mask, tinted per character by the sprite shader.
			rect(16, 25 + bob, 16, 22, 0, 0, 255);
			rect(14, 36 + bob, 20, 10, 0, 0, 255);
			rect(16, 27 + bob, 2, 12, 0, 0, 180);
			rect(14, 45 + bob, 20, 2, 0, 0, 180);
			rect(14, 39 + bob, 20, 3, 80, 52, 30);
			rect(23, 39 + bob, 4, 3, 218, 173, 79);
			oval(24, 17 + bob, 9, 11, 39, 32, 26);
			oval(24, 19 + bob, 7, 9, 219, 170, 116);
			oval(23, 11 + bob, 9, 5, 64, 45, 29);
			rect(16, 12 + bob, 3, 8, 64, 45, 29);

			if (direction == 3)
			{
				oval(24, 17 + bob, 8, 9, 65, 46, 29);
				rect(18, 28 + bob, 13, 14, 102, 66, 36);
				rect(20, 28 + bob, 9, 4, 163, 118, 64);
				rect(23, 29 + bob, 2, 12, 200, 158, 90);
			}
			else
			{
				int eye = direction == 1 ? 20 : 26;
				rect(eye, 18 + bob, 2, 2, 34, 39, 34);

				if (direction == 1)
					rect(27, 18 + bob, 2, 2, 34, 39, 34);
				rect(24, 25 + bob, 3, 1, 137, 86, 58);

				// Cowl, shoulder strap and leather satchel.
				rect(17, 27 + bob, 14, 3, 205, 165, 91);

				for (int i = 0; i < 13; ++i)
					rect(16 + i, 30 + bob + i / 2, 2, 2, 109, 73, 37);
				rect(30, 36 + bob, 7, 9, 112, 72, 36);
				rect(31, 36 + bob, 5, 2, 178, 128, 68);
			}
		}
	SaveAssetCache(L"character-v1.rgba", 1, pixels);
	return pixels;
}
