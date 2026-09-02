cmake_minimum_required(VERSION 3.21)
project(bookward VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- dataward (typed persistence) -------------------------------------------
# Pinned to the SDK-core-complete commit; re-pin deliberately, not via `main`.
include(FetchContent)
FetchContent_Declare(dataward
  GIT_REPOSITORY https://github.com/bdcbqa314159/dataward.git
  GIT_TAG f3e5421af488f694c08d9c6f2709bda93b7359dc)
FetchContent_MakeAvailable(dataward)

# Core logic as a library so tests can drive it; main.cpp stays thin.
add_library(bookward_core src/commands.cpp src/commands_impl.cpp)
target_include_directories(bookward_core PUBLIC include)
target_link_libraries(bookward_core PUBLIC dataward::dataward)
if(MSVC)
  target_compile_options(bookward_core PRIVATE /W4)
else()
  target_compile_options(bookward_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bookward src/main.cpp)
target_link_libraries(bookward PRIVATE bookward_core)

option(BOOKWARD_BUILD_TESTS "Build bookward's tests" ${PROJECT_IS_TOP_LEVEL})
if(BOOKWARD_BUILD_TESTS)
  enable_testing()
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2 GIT_SHALLOW TRUE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(bookward_tests tests/commands_test.cpp)
  target_link_libraries(bookward_tests PRIVATE bookward_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(bookward_tests)
endif()
