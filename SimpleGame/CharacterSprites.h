#pragma once
#include <vector>

// Nine 48x64 cells per facing: eight walk frames and one resting pose.
constexpr int SpriteWidth = 48, SpriteHeight = 64, SpriteFrames = 9, SpriteDirections = 4;
std::vector<unsigned char> CreateCharacterAtlas();
