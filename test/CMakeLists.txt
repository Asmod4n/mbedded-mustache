find_package(simdjson REQUIRED)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(spec spec.cpp)
target_link_libraries(spec PRIVATE mustache::mustache mustache::mustache-c simdjson::simdjson GTest::gtest_main)
target_compile_definitions(spec PRIVATE MUSTACHE_SPEC_DIR="${PROJECT_SOURCE_DIR}/refs/mustache-spec")
gtest_discover_tests(spec)

add_executable(output output.cpp)
target_link_libraries(output PRIVATE mustache::mustache mustache::mustache-c GTest::gtest_main)
gtest_discover_tests(output)

add_library(c_api_checks OBJECT c_api.c)
target_link_libraries(c_api_checks PRIVATE mustache::mustache-c)
set_target_properties(c_api_checks PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
add_executable(c_api c_api_test.cpp $<TARGET_OBJECTS:c_api_checks>)
target_link_libraries(c_api PRIVATE mustache::mustache-c GTest::gtest_main)
gtest_discover_tests(c_api)

add_executable(escape escape.cpp)
target_link_libraries(escape PRIVATE mustache::mustache GTest::gtest_main)
gtest_discover_tests(escape)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#ifndef __cpp_impl_reflection\n#error no reflection\n#endif\nint main() { return 0; }" MUSTACHE_HAS_REFLECTION)
if(MUSTACHE_HAS_REFLECTION)
  add_executable(reflect reflect.cpp)
  target_link_libraries(reflect PRIVATE mustache::mustache GTest::gtest_main)
  gtest_discover_tests(reflect)
endif()
