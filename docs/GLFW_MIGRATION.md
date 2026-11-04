# GLFW 전환과 GPU 폰트 데이터 시스템

2026-11-04 교수님 강의 지시 작업. 스토리 개발 3단계를 완료 처리하는 작업이 아니다.

## 전환 범위

| 기존 기능 | 현재 처리 |
|---|---|
| FreeGLUT 창·메인 루프 | GLFW 창, PollEvents → 기존 갱신/그리기 → SwapBuffers |
| 키 입력·키 해제 | GLFW 콜백을 기존 256-key 상태와 KeyDown/KeyUp에 연결 |
| 키 반복 | GLFW_REPEAT를 무시하여 E·J·Esc 등의 중복 토글 방지 |
| 달리기 | 왼쪽/오른쪽 Shift 상태를 GLFW로 읽고 기존 Sprint 슬롯 16에 반영 |
| 포커스·크기·종료 | 포커스 상실 시 입력 비움, framebuffer 픽셀 크기 반영, 최소 창 크기 유지, GPU 자원 해제 후 창 파괴 |
| VSync | glfwSwapInterval(1), 기존 steady_clock과 1/120초 충돌 단계·0.25초 지연 상한 유지 |
| WGL 글자 표시 목록 | 글리프 메트릭·아틀라스 좌표 캐시, R8 텍스처와 기존 정점 버퍼를 사용한 GPU 그리기 |
| 미리 빌드된 lib/DLL | 포함한 GLFW/GLEW C 소스를 구성별로 정적 빌드 |
| Windows 전용 파일·문자·통신 | 실행 파일/캐시 경로와 UTF-8 변환, WinSock/POSIX 소켓 분기 |

FreeGLUT에 기대던 기능이 전환 뒤에도 이어지도록 한 작업이다. 교수님이 제공한 별도 Font 클래스나 수업 예제는 확인하지 못했으며, 사용자 설명에 따라 폰트를 'GPU가 읽는 데이터와 이를 관리하는 시스템'의 관점으로 구현했다.

Windows/Linux의 기본 OpenGL 3.3 Compatibility 프로파일은 기존과 같다. macOS만 지원 조건에 맞춰 Core/forward-compatible 경로를 쓴다. MSAA·sRGB를 새로 켜지 않았고 깊이/스텐실 없는 기존 화면 구성을 유지한다. 자동 Retina framebuffer 확대를 끄고 기본 픽셀 예산을 유지한다. GPU 컨텍스트가 살아 있는 동안 렌더러를 해제한 뒤 창을 닫는다.

## GPU 폰트 데이터와 메모리

### 데이터 흐름

FontRasterizer의 CPU 데이터 → GlyphBitmap → Renderer의 Glyph 캐시 → 단일 R8 아틀라스와 ModelVertex 정점 → 기존 OpenGL 버퍼/셰이더.

- GlyphBitmap: 글자 폭·높이, 기준선 오프셋, 다음 글자까지의 이동량, 일시적인 알파 비트맵.
- Glyph: 위 메트릭과 GPU 아틀라스 내 배치 좌표를 문자 코드별로 보관한다.
- ModelVertex: 위치·색·Font 머티리얼 식별자·UV를 기존 정점 구조로 전달한다.
- 글리프를 처음 요청할 때만 래스터화하고 glTexSubImage2D로 해당 부분을 올린다. 같은 글자는 캐시를 즉시 반환한다.
- 아틀라스는 2048×2048의 GL_R8 하나로 고정하며 원시 저장량은 4,194,304바이트(4 MiB)다. 공간이 가득 차면 미리 확보한 대체 글자를 사용한다.
- 글자마다 GPU 표시 목록이나 텍스처를 만들지 않는다. 본문·패널 등 UI의 기존 순서를 보존하면서 정점 묶음으로 제출한다.
- TTF 데이터는 렌더러 초기화 시 메모리에 읽으며 매 프레임 다시 파일을 열지 않는다. 비트맵 임시 메모리는 업로드 뒤 해제한다.
- 렌더러 종료 시 GPU 텍스처·버퍼와 CPU 캐시, Windows의 글꼴/DC를 함께 정리한다. 창 리사이즈와 지역 이동은 폰트 캐시를 다시 만들지 않는다.

Windows에서는 기존 맑은 고딕의 -18px·굵기·글자 이동량과 y+18 기준선을 유지한다. WGL에 종속된 출력 호출은 없앴고, 동일한 GDI 비트맵을 GPU 아틀라스에 올린다. 시스템 폰트를 쓸 수 없거나 해당 글자가 없으면 저장소에 포함한 나눔고딕/stb_truetype 경로를 사용한다. 다른 OS의 대체 폰트는 모양·안티앨리어싱이 Windows 시스템 글꼴과 다를 수 있다.

## 다른 PC에서 빌드와 배포

[루트 빌드 설명](../README.md)을 따른다. GLFW 3.4, GLEW 2.3.1, stb 구현, 폰트와 라이선스를 포함한다. CMake 구성에서 네트워크 다운로드나 설치된 GLFW/GLEW 패키지를 요구하지 않는다.

기존 솔루션은 GameDependencies.vcxproj의 C 소스와 게임 C++17 소스를 따로 컴파일한다. C 파일에 게임의 C++ PCH를 강제로 적용하지 않는다. x64/x86과 Debug/Release의 CRT를 각각 맞추고 /MT 또는 /MTd로 빌드한다. Shaders·Fonts·Monitor와 라이선스 고지를 실행 파일 옆에 복사한다.

Linux는 X11/GLX 경로를 기본으로 하며 Wayland 전용 빌드를 이 구성에서 제공하지 않는다. OS의 컴파일러·SDK·OpenGL 드라이버/개발 패키지까지 저장소에 재배포하는 것은 아니다. 모델/스프라이트 캐시 형식과 퀘스트·전투·지역 데이터는 그대로다.

## 검증 기록

- 기존 Visual Studio 솔루션: x64/x86 × Debug/Release 네 구성 컴파일·링크 및 실행 자원 복사 통과.
- PE 의존성 검사: WS2_32, KERNEL32, GDI32, SHELL32, OPENGL32, USER32만 확인. FreeGLUT/GLFW/GLEW/VC++ 런타임 DLL의 별도 배포 의존 없음.
- CMake x64: Debug/Release 게임 및 검사 코드 빌드, 독립 회귀 검사 각 11개 통과. 224개 지역 맵, 두 드워프 선택, 성장·전리품·NPC·SceneGraph·충돌·입력·폰트 검사를 포함.
- 로컬 JSON 프로토콜 검사: UTF-8/이모지/로캘, 자원 경로, 집계, 503 복구, 부분 요청·제한·종료 처리 통과.
- 숨긴 GLFW 창: Compatibility 및 Core 컨텍스트에서 셰이더·한글·마을·드워프 도시 렌더링 통과.
- 기존 WGL과 아틀라스 출력의 비교 문자열 '갈대여울 ABC123 gjpqy!? →': RGBA 4,096,000채널 중 차이 0. 전체 게임 화면의 픽셀 동일성을 뜻하지 않는다.
- 같은 마을 장면 반복 그리기에서 글리프 캐시 수·아틀라스 배치 좌표·텍스처 핸들·정점 버퍼 용량이 유지됨을 확인.
- 로컬 원시 로그: x64/GlfwMigration/. 버전 관리 대상은 아니다. 실행 환경의 하위 프로세스 쓰기 제약 때문에 검사용 빌드 출력은 임시 폴더를 사용했다.

숨긴 창의 렌더 배치 측정은 게임 전체 FPS나 사용자 플레이에서의 최저 프레임 보장이 아니다. 실제 키 감각·화면 배율·후처리·장시간 플레이/FPS, Linux/macOS의 빌드·실행은 사용자 또는 해당 OS에서 별도 확인해야 한다. 이 작업에서 일반 게임 창을 열어 직접 플레이하지 않았다.

## 근거 자료

- [GLFW 공식 빌드 가이드](https://www.glfw.org/docs/3.4/compile_guide.html)
- [GLFW 창·프레임버퍼·프로파일 가이드](https://www.glfw.org/docs/3.4/window_guide.html)
- [GLFW 애플리케이션 빌드 가이드](https://www.glfw.org/docs/3.4/build_guide.html)
- 라이브러리와 폰트의 출처·버전·라이선스: [ThirdParty](../ThirdParty/README.md), versions.json, THIRD_PARTY_NOTICES.txt.
