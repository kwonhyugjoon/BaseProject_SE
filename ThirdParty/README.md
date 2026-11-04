# Included dependencies

The implementations needed for an offline build are checked in. Upstream files are unmodified; their original formatting and line endings are retained.

| Component | Included implementation | License |
|---|---|---|
| GLFW 3.4 | include, src, CMake, deps, upstream CMakeLists.txt | zlib-style, glfw/LICENSE.md |
| GLEW 2.3.1 | src/glew.c, include/GL | glew/LICENSE.txt (BSD/MIT/Khronos notices) |
| stb_truetype | stb/stb_truetype.h, pinned revision | stb/LICENSE (MIT/public domain) |
| Nanum Gothic Regular | ../SimpleGame/Fonts/NanumGothic-Regular.ttf | ../SimpleGame/Fonts/OFL.txt (SIL OFL) |

versions.json records release archive hashes, exact stb/font revisions, and the font SHA-256. Windows system fonts are used when present and are not redistributed.

Sources:
- https://github.com/glfw/glfw/releases/tag/3.4
- https://github.com/nigels-com/glew/releases/tag/glew-2.3.1
- https://github.com/nothings/stb
- https://github.com/google/fonts/tree/main/ofl/nanumgothic

CMake builds GLFW and GLEW as static libraries. GameDependencies.vcxproj builds the same Win32 implementation sources for the original Visual Studio solution. Third-party C sources compile separately as C without the game's C++ precompiled header. No prebuilt library or DLL is required.

The combined THIRD_PARTY_NOTICES.txt at the repository root is copied beside the game executable. The font's OFL notice is also copied with the font.
