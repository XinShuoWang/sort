cmake_minimum_required(VERSION 3.25)

project(tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(GTest CONFIG REQUIRED)
set(TEST_LINK_LIBS pthread PkgConfig::liburing glog::glog fmt::fmt GTest::gtest GTest::gtest_main)

option(ENABLE_COVERAGE "Enable coverage" ON)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

file(GLOB LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cc)
file(GLOB TEST_SOURCES *.cc)

foreach(test_src ${TEST_SOURCES})
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src} ${LIB_SOURCES})
  target_link_libraries(${test_name} PRIVATE ${TEST_LINK_LIBS})
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
