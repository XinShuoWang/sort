cmake_minimum_required(VERSION 3.25)

project(sort CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, e.g. Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()

include_directories(include)

include(FindPkgConfig)
pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)
find_package(glog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

set(LINK_LIBS pthread PkgConfig::liburing glog::glog fmt::fmt)

add_executable(min_demo src/demo.cc)
target_link_libraries(min_demo PRIVATE ${LINK_LIBS})

add_executable(sort src/sort.cc)
target_link_libraries(sort PRIVATE ${LINK_LIBS})