cmake_minimum_required(VERSION 3.16)
project(ksynth-desktop C CXX)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

# ── platform audio dependencies ──────────────────────────────────────
if(APPLE)
    find_library(COREAUDIO      CoreAudio      REQUIRED)
    find_library(COREFOUNDATION CoreFoundation REQUIRED)
    find_library(AUDIOUNIT      AudioUnit      REQUIRED)
    find_library(AUDIOTOOLBOX   AudioToolbox   REQUIRED)
    set(PLATFORM_LIBS ${COREAUDIO} ${COREFOUNDATION} ${AUDIOUNIT} ${AUDIOTOOLBOX})
elseif(WIN32)
    set(PLATFORM_LIBS ole32 uuid winmm)
elseif(UNIX)
    # miniaudio selects the best available backend (PulseAudio, ALSA, JACK)
    # at runtime via dlopen — we just need pthread and dl.
    set(PLATFORM_LIBS pthread dl m)
endif()

# ── common sources (engine + audio, no FLTK) ────────────────────────
set(CORE_SOURCES
    src/engine.c
    src/audio.c
)

# ── Phase 1: headless CLI ────────────────────────────────────────────
add_executable(ksplay
    src/main.c
    ${CORE_SOURCES}
)
target_include_directories(ksplay PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
)
target_link_libraries(ksplay PRIVATE ${PLATFORM_LIBS})
if(UNIX)
    target_link_libraries(ksplay PRIVATE m)
endif()
target_compile_options(ksplay PRIVATE -Wall -Wno-unused-function)

# ── vendored FLTK 1.4 ────────────────────────────────────────────────
set(FLTK_BUILD_TEST          OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_FLUID         OFF CACHE BOOL "" FORCE)
set(FLTK_BUILD_FLTK_OPTIONS  OFF CACHE BOOL "" FORCE)
set(OPTION_BUILD_HTML_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(OPTION_BUILD_PDF_DOCUMENTATION  OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/fltk EXCLUDE_FROM_ALL)

# ── Phase 2+: FLTK UI app ────────────────────────────────────────────
add_executable(ksynth-desktop
    src/main_ui.cpp
    src/theme.cpp
    src/ui/MainWindow.cpp
    src/ui/SlotStrip.cpp
    src/ui/Editor.cpp
    src/ui/Notebook.cpp
    src/ui/PadGrid.cpp
    src/ui/PadWindow.cpp
    ${CORE_SOURCES}
)
target_link_libraries(ksynth-desktop PRIVATE fltk ${PLATFORM_LIBS})
target_include_directories(ksynth-desktop PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/vendor/fltk
    ${CMAKE_BINARY_DIR}/vendor/fltk
)
target_compile_options(ksynth-desktop PRIVATE -Wall -Wno-unused-function)
