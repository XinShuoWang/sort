cmake_minimum_required(VERSION 3.25)

project(sort CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, e.g. Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()

add_executable(min_demo demo.cc)
target_link_libraries(min_demo PRIVATE pthread)

find_package(fmt CONFIG REQUIRED)
add_executable(sort sort.cc)
target_link_libraries(sort PRIVATE pthread)