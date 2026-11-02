cmake_minimum_required(VERSION 3.15)
project(CI5Mod LANGUAGES C CXX)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "CI5Mod must be built for x86 (use build.bat or -A Win32).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure 32-bit target if MSVC
if(MSVC)
    add_compile_options(/W3 /MP /EHsc)
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")
set(MINHOOK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/minhook")

set(SOURCES
    src/dllmain.cpp
    src/core/memory.cpp
    src/core/hooks.cpp
    src/game/game_manager.cpp
    src/features/feature_manager.cpp
    src/features/stats_feature.cpp
    src/features/survival_feature.cpp
    src/features/weapon_feature.cpp
    src/features/teleport_feature.cpp
    src/ui/gui.cpp
    
    # ImGui
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_dx9.cpp
    ${IMGUI_DIR}/imgui_impl_win32.cpp

    # MinHook
    ${MINHOOK_DIR}/buffer.c
    ${MINHOOK_DIR}/trampoline.c
    ${MINHOOK_DIR}/hook.c
    ${MINHOOK_DIR}/hde32.c
)

add_library(CI5Mod SHARED ${SOURCES})

target_include_directories(CI5Mod PRIVATE
    src
    ${IMGUI_DIR}
    ${MINHOOK_DIR}
    ${MINHOOK_DIR}/include
)

target_link_libraries(CI5Mod PRIVATE
    d3d9
    user32
    gdi32
    kernel32
)

set_target_properties(CI5Mod PROPERTIES
    OUTPUT_NAME "CI5Mod"
    PREFIX ""
)

add_executable(CI5Injector tools/injector.cpp)
set_target_properties(CI5Injector PROPERTIES
    OUTPUT_NAME "CI5Injector"
)

option(CI5MOD_BUILD_TESTS "Build encoded-stat regression tests" OFF)
if(CI5MOD_BUILD_TESTS)
    enable_testing()
    add_executable(encoded_int_tests tests/encoded_int_tests.cpp)
    target_include_directories(encoded_int_tests PRIVATE src)
    add_test(NAME encoded_int COMMAND encoded_int_tests)
endif()

option(CI5MOD_LIVE_TEST "Expose a reversible native stat round-trip test" OFF)
if(CI5MOD_LIVE_TEST)
    target_compile_definitions(CI5Mod PRIVATE CI5MOD_LIVE_TEST)
    target_sources(CI5Mod PRIVATE tests/live_test.cpp)
endif()
