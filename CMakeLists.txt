cmake_minimum_required(VERSION 3.0)
project(rtlog VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17) # TODO: we only use 17 for CTAD type things, can we get away with 11 ?
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add library header files
set(HEADERS
  include/rtlog/rtlog.h
)

# Create library target
add_library(rtlog INTERFACE ${HEADERS})
add_library(rtlog::rtlog ALIAS rtlog)

# Set include directories for library
target_include_directories(rtlog INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Set project include directories
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Try to find the package ReaderWriterQueue, if not found fetch with FetchContent
find_package(ReaderWriterQueue QUIET)
if(NOT TARGET readerwriterqueue)
    include(FetchContent)
    FetchContent_Declare(ReaderWriterQueue
        GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue
    )
    FetchContent_MakeAvailable(ReaderWriterQueue)
endif()

if(NOT TARGET stb::stb)
    # Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
        cmake_policy(SET CMP0135 NEW)
    endif()

    include(FetchContent)
    FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb
    )
    # Note we do not "MakeAvailable" here, because stb is not cmake, just populate 
    if(NOT stb_POPULATED)
        FetchContent_Populate(stb)
    endif()

    add_library(stb INTERFACE)
    add_library(stb::stb ALIAS stb)

    target_include_directories(stb INTERFACE
        $<BUILD_INTERFACE:${stb_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
endif()

if (RTLOG_USE_FMTLIB AND NOT TARGET fmt::fmt)
    include(FetchContent)

    FetchContent_Declare(fmtlib
        GIT_REPOSITORY https://github.com/fmtlib/fmt 
    )
    FetchContent_MakeAvailable(fmtlib)
endif()

target_link_libraries(rtlog 
    INTERFACE 
        readerwriterqueue
        stb::stb
        $<$<BOOL:${RTLOG_USE_FMTLIB}>:fmt::fmt>
)

target_compile_definitions(rtlog 
    INTERFACE 
        STB_SPRINTF_IMPLEMENTATION 
        $<$<BOOL:${RTLOG_USE_FMTLIB}>:RTLOG_USE_FMTLIB>
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
)

target_compile_options(rtlog 
    INTERFACE 
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-O2>
)

option(RTLOG_BUILD_TESTS "Build tests" ON)
if(RTLOG_BUILD_TESTS)
    include(CTest)
    set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
    enable_testing()
    add_subdirectory(test)
endif()

option(RTLOG_BUILD_EXAMPLES "Build examples" ON)
if(RTLOG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# TODO: WTF is installing ??
# Install library
#install(TARGETS rtlog
#  EXPORT rtlogTargets
#  INCLUDES DESTINATION include
#)
#
## Install library header files
#install(DIRECTORY include/rtlog
#  DESTINATION include
#)
#
## Install CMake config files
#install(EXPORT rtlogTargets
#  FILE rtlogConfig.cmake
#  NAMESPACE rtlog::
#  DESTINATION lib/cmake/rtlog
#)


