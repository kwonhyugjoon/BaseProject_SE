# BaseProject_SE

C++17 / OpenGL 3.3 / GLFW 기반 쿼터뷰 2.5D 싱글플레이 RPG 프로토타입이다.

## Visual Studio

Visual Studio 2022의 'C++를 사용한 데스크톱 개발'과 Windows SDK가 필요하다.

1. SimpleGame.sln을 열고 Release | x64를 선택한다.
2. 솔루션을 빌드하면 GameDependencies 프로젝트가 포함된 GLFW·GLEW 소스를 먼저 정적 빌드한다.
3. SimpleGame을 실행한다. Debug와 x86 구성도 지원한다.

외부 FreeGLUT·GLFW·GLEW DLL이나 별도 라이브러리 다운로드는 필요하지 않다. MSVC 런타임도 정적 링크한다. 실행 파일과 함께 복사되는 Shaders, Fonts, Monitor 폴더와 THIRD_PARTY_NOTICES.txt를 배포한다. 시스템의 OpenGL 드라이버는 필요하다.

## CMake

CMake 3.20 이상과 C++17 컴파일러를 사용한다. 소스와 라이선스를 저장소에 포함했으므로 구성·빌드 중 인터넷 다운로드를 수행하지 않는다.

Windows / Visual Studio 2022:

```powershell
cmake --preset vs2022-x64
cmake --build --preset release-x64
ctest --preset release-x64
```

실행 파일: out/build/vs2022-x64/bin/Release/SimpleGame.exe.
Debug는 debug-x64 빌드·검사 프리셋을 사용한다. 32비트는 vs2022-x86 구성과 release-x86 / debug-x86 빌드 프리셋을 사용한다. Visual Studio의 CMake 도구를 설치했지만 PATH에 없으면 개발자 프롬프트에서 실행하거나 cmake.exe의 전체 경로를 사용한다.

Linux / macOS:

```sh
cmake -S . -B out/build/native -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/native --parallel 4
ctest --test-dir out/build/native --output-on-failure
./out/build/native/bin/Release/SimpleGame
```

Linux는 X11 백엔드와 OpenGL 개발 패키지가 필요하다. Ubuntu/Debian의 X11 의존성은 xorg-dev 메타 패키지로 마련할 수 있다. macOS는 Xcode Command Line Tools와 CMake가 필요하며 Cocoa/Core OpenGL 경로를 사용한다. Linux·macOS의 실제 빌드/실행은 아직 검증하지 않았다. [GLFW 공식 빌드 조건](https://www.glfw.org/docs/3.4/compile_guide.html)을 참고한다.

## 확인과 개발 문서

- tests/RunTravelChecks.cmd: Windows에서 CMake로 빌드하고 독립 회귀 검사를 실행한다. 첫 인자로 Debug를 지정할 수 있다.
- GlfwRenderTests: 화면에 보이지 않는 GLFW 창으로 렌더링·문자 출력·캐시를 검사한다. --core 인자로 Core 프로파일을 검사한다. 디스플레이와 OpenGL 드라이버가 필요한 검사라 일반 ctest에서는 자동 실행하지 않는다.
- [GLFW 전환과 GPU 폰트 시스템](docs/GLFW_MIGRATION.md)
- [게임 개발 기준](docs/GAME_DESIGN.md), [10회 개발 계획](docs/DEVELOPMENT_ROADMAP.md)
- [지역 이동](docs/WORLD_TRAVEL.md), [드워프 이야기](docs/DWARF_STORY.md), [고원과 고룡 이야기](docs/DRAGON_STORY.md)
