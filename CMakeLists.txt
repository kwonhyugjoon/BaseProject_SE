cmake_minimum_required(VERSION 3.20)
project(BaseProject_SE VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# All third-party implementations are vendored: configure never downloads code.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
add_subdirectory(ThirdParty/glfw EXCLUDE_FROM_ALL)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
add_library(game_glew STATIC ThirdParty/glew/src/glew.c)
target_include_directories(game_glew PUBLIC ThirdParty/glew/include)
target_compile_definitions(game_glew PUBLIC GLEW_STATIC GLEW_NO_GLU)
target_link_libraries(game_glew PUBLIC OpenGL::GL ${CMAKE_DL_LIBS})

add_library(game_core STATIC
    SimpleGame/Actor.cpp SimpleGame/SceneGraph.cpp SimpleGame/AssetCache.cpp
    SimpleGame/LevelModels.cpp SimpleGame/CharacterSprites.cpp SimpleGame/Renderer.cpp
    SimpleGame/FontRasterizer.cpp SimpleGame/PlatformPaths.cpp
    SimpleGame/FirstLevel.cpp SimpleGame/QuestJournal.cpp
    SimpleGame/MonitorState.cpp SimpleGame/MonitorServer.cpp)
target_include_directories(game_core PUBLIC SimpleGame)
target_compile_definitions(game_core PUBLIC NOMINMAX)
target_link_libraries(game_core PUBLIC glfw game_glew Threads::Threads)
if(WIN32)
    target_link_libraries(game_core PUBLIC gdi32 user32 shell32 ws2_32)
endif()
if(MSVC)
    target_compile_options(game_core PUBLIC /utf-8)
endif()

add_custom_target(game_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/SimpleGame/Shaders" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/SimpleGame/Monitor" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Monitor"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/SimpleGame/Fonts" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.txt" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/THIRD_PARTY_NOTICES.txt")
function(game_assets target)
    add_dependencies(${target} game_resources)
endfunction()
add_executable(SimpleGame SimpleGame/SimpleGame.cpp)
target_link_libraries(SimpleGame PRIVATE game_core)
game_assets(SimpleGame)

include(CTest)
if(BUILD_TESTING)
    foreach(name SprintTests QuestJournalTests WorldTravelTests RegionConnectivityTests
                 LevelExpansionTests TutorialPopulationTests DwarfStoryTests
                 SceneGraphTests TreeCollisionGridTests FontTests GlfwInputTests)
        add_executable(${name} tests/${name}.cpp)
        target_link_libraries(${name} PRIVATE game_core)
        # Assertions are required even when testing the Release build.
        if(MSVC)
            target_compile_options(${name} PRIVATE /UNDEBUG)
        else()
            target_compile_options(${name} PRIVATE -UNDEBUG)
        endif()
        game_assets(${name})
        add_test(NAME ${name} COMMAND ${name})
        set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            ENVIRONMENT "LOCALAPPDATA=${CMAKE_CURRENT_BINARY_DIR}/test-cache;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-cache")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-cache")
    endforeach()
    add_executable(GlfwRenderTests tests/GlfwRenderTests.cpp)
    target_link_libraries(GlfwRenderTests PRIVATE game_core)
    if(MSVC)
        target_compile_options(GlfwRenderTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(GlfwRenderTests PRIVATE -UNDEBUG)
    endif()
    game_assets(GlfwRenderTests)
    add_executable(MonitorServerTestHost tests/MonitorServerTestHost.cpp)
    target_link_libraries(MonitorServerTestHost PRIVATE game_core)
    if(MSVC)
        target_compile_options(MonitorServerTestHost PRIVATE /UNDEBUG)
    else()
        target_compile_options(MonitorServerTestHost PRIVATE -UNDEBUG)
    endif()
    game_assets(MonitorServerTestHost)
    # GPU/display-dependent test: run explicitly; headless ctest stays portable.
endif()
