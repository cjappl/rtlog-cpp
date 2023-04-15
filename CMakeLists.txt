cmake_minimum_required(VERSION 3.0)
project(rtlog VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
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

# Install library
install(TARGETS rtlog
  EXPORT rtlogTargets
  INCLUDES DESTINATION include
)

# Install library header files
install(DIRECTORY include/rtlog
  DESTINATION include
)

# Install CMake config files
install(EXPORT rtlogTargets
  FILE rtlogConfig.cmake
  NAMESPACE rtlog::
  DESTINATION lib/cmake/rtlog
)

# Set project include directories
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

option(RTLOG_BUILD_TESTS "Build tests" ON)
if(RTLOG_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
