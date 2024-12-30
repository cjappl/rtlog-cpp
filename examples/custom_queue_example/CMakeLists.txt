find_package(ReaderWriterQueue QUIET)
if(NOT TARGET readerwriterqueue)
    include(FetchContent)
    FetchContent_Declare(ReaderWriterQueue
        GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue
    )
    FetchContent_MakeAvailable(ReaderWriterQueue)
endif()

add_executable(custom_queue_example
  customqueuemain.cpp
)

target_link_libraries(custom_queue_example
  PRIVATE
    rtlog::rtlog
    readerwriterqueue
)
