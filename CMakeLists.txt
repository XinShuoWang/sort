cmake_minimum_required(VERSION 3.25)

project(sort CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, e.g. Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()

include_directories(include)

add_executable(min_demo src/demo.cc)
target_link_libraries(min_demo PRIVATE pthread)

add_executable(sort src/sort.cc)
target_link_libraries(sort PRIVATE pthread)

#add_executable(hash_table src/hashTable.cc)
#target_link_libraries(hash_table PRIVATE pthread)