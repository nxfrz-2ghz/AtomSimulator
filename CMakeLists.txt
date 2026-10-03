cmake_minimum_required(VERSION 3.14)
project(AtomSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Без Release симуляция работает в разы медленнее (нет оптимизаций).
# Для IDE с несколькими конфигурациями (Visual Studio, Xcode) это не нужно.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------- raylib ----------------
include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        6.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

find_package(OpenMP)

# ---------------- Программа ----------------
add_executable(AtomSimulator
    main.cpp
    src/manager/manager.cpp
    src/render/render.cpp
    src/simulation/simulation.cpp
)

target_link_libraries(AtomSimulator PRIVATE raylib)

if(OpenMP_CXX_FOUND)
    target_link_libraries(AtomSimulator PRIVATE OpenMP::OpenMP_CXX)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(AtomSimulator PRIVATE -fopenmp)
    endif()
    message(STATUS "OpenMP: enabled")
else()
    message(STATUS "OpenMP: not found, building single-threaded")
endif()

if(MSVC)
    target_compile_options(AtomSimulator PRIVATE /W3)
else()
    target_compile_options(AtomSimulator PRIVATE -Wall)
endif()
