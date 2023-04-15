include(FetchContent)
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest
)
FetchContent_MakeAvailable(doctest)

# These two are painfully not easily accessed. 
# TODO: CAPPLE submitted a PR that got accepted into doctest, switch to that newer version when we can
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
set(_DOCTEST_DISCOVER_TESTS_SCRIPT "${_DOCTEST_DISCOVER_TESTS_SCRIPT}" CACHE PATH "Path to doctest cmake helper file" FORCE)

target_compile_definitions(doctest 
    INTERFACE
        DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
        DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
)

add_executable(rtlog_tests test_rtlog.cpp)

target_link_libraries(rtlog_tests 
    PRIVATE 
        doctest::doctest 
        rtlog::rtlog
)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED ON)

if (CMAKE_GENERATOR STREQUAL "Xcode" OR NOT APPLE)
    add_test(NAME rtlog_tests COMMAND rtlog_tests)
else()
    doctest_discover_tests(rtlog_tests)
endif()


