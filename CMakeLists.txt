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
find_package(zstd CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

set(LINK_LIBS pthread PkgConfig::liburing glog::glog fmt::fmt lz4::lz4 zstd::libzstd)

file(GLOB SRC_FILES src/*.cc)

add_library(BufferManager ${SRC_FILES})
target_link_libraries(BufferManager ${LINK_LIBS})

# demo 程序已迁移为单元测试，不再构建可执行

add_subdirectory(test)
