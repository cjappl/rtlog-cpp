cmake_minimum_required(VERSION 3.6)
project(rtlog VERSION 1.0.0)

option(RTLOG_USE_FMTLIB "Use fmtlib for formatting" OFF)
option(RTLOG_FULL_WARNINGS "Enable full warnings" OFF)
option(RTLOG_BUILD_TESTS "Build tests" OFF)
option(RTLOG_BUILD_EXAMPLES "Build examples" OFF)


set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fsanitize=realtime" COMPILER_SUPPORTS_RTSAN)
unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
if(COMPILER_SUPPORTS_RTSAN)
  message(STATUS "Compiler supports -fsanitize=realtime, allowing RTLOG_USE_RTSAN option.")
  option(RTLOG_USE_RTSAN "Use -fsanitize=realtime" OFF)
endif()

# Add library header files
set(HEADERS
  include/rtlog/rtlog.h
)

# Create library target
add_library(rtlog INTERFACE)
add_library(rtlog::rtlog ALIAS rtlog)

if (CMAKE_CXX_STANDARD LESS 17)
  message(WARNING "C++17 or higher is required for rtlog. Setting C++17 for the target..")
endif()

target_compile_features(rtlog INTERFACE cxx_std_17)

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

if(NOT RTSAN_USE_FMTLIB AND NOT TARGET stb::stb)
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
        GIT_TAG 11.0.2
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
        $<$<NOT:$<BOOL:${RTLOG_USE_FMTLIB}>>:RTLOG_USE_STB>
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
)

set(RTLOG_ALL_WARNINGS)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(RTLOG_ALL_WARNINGS "-Wall;-Werror;-Wformat;-Wextra;-Wformat-security;-Wno-unused-function")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(RTLOG_ALL_WARNINGS "/W4;/WX;/wd4505")
endif()

target_compile_options(rtlog 
    INTERFACE 
        $<$<BOOL:${RTLOG_FULL_WARNINGS}>:${RTLOG_ALL_WARNINGS}>
        $<$<BOOL:${RTLOG_USE_RTSAN}>:-fsanitize=realtime>
)

target_link_options(rtlog
    INTERFACE
        $<$<BOOL:${RTLOG_USE_RTSAN}>:-fsanitize=realtime>
)

if(RTLOG_BUILD_TESTS)
    include(CTest)
    set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
    enable_testing()
    add_subdirectory(test)
endif()

if(RTLOG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# TODO: figure out installing
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
