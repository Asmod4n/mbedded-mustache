find_package(simdjson REQUIRED)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(spec spec.cpp)
target_link_libraries(spec PRIVATE mustache::mustache simdjson::simdjson GTest::gtest_main)
target_compile_definitions(spec PRIVATE MUSTACHE_SPEC_DIR="${PROJECT_SOURCE_DIR}/refs/mustache-spec")
gtest_discover_tests(spec)

add_executable(output output.cpp)
target_link_libraries(output PRIVATE mustache::mustache GTest::gtest_main)
gtest_discover_tests(output)

add_executable(limits limits.cpp)
target_link_libraries(limits PRIVATE mustache::mustache GTest::gtest_main)
gtest_discover_tests(limits)

add_executable(media_type media_type.cpp)
target_link_libraries(media_type PRIVATE mustache::mustache GTest::gtest_main)
gtest_discover_tests(media_type)

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

find_package(Boost CONFIG QUIET)
if(MUSTACHE_HAS_REFLECTION OR Boost_FOUND)
  add_executable(typed typed.cpp)
  target_link_libraries(typed PRIVATE mustache::mustache GTest::gtest_main)
  if(Boost_FOUND)
    target_link_libraries(typed PRIVATE Boost::headers)
  endif()
  gtest_discover_tests(typed)
endif()
