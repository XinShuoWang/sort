enable_testing()

option(ENABLE_COVERAGE "Enable coverage" ON)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

find_package(GTest CONFIG REQUIRED)
set(GTEST_LINK_LIBS GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

file(GLOB TEST_SOURCES *.cc)

foreach(test_src ${TEST_SOURCES})
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_link_libraries(${test_name} PRIVATE BufferManager ${GTEST_LINK_LIBS})
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
