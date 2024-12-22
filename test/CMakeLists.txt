if (NOT TARGET gtest_main)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(rtlog_tests test_rtlog.cpp)

target_link_libraries(rtlog_tests 
    PRIVATE 
        gtest_main 
        rtlog::rtlog
)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED ON)

include(GoogleTest)
gtest_discover_tests(rtlog_tests)
