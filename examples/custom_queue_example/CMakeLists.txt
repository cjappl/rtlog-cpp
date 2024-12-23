if (NOT TARGET farbot)
    include(FetchContent)

    FetchContent_Declare(farbot
        GIT_REPOSITORY https://github.com/hogliux/farbot
        GIT_TAG 0416705394720c12f0d02e55c144e4f69bb06912
    )
    # Note we do not "MakeAvailable" here, because farbot does not fully work via FetchContent
    if(NOT farbot_POPULATED)
        FetchContent_Populate(farbot)
    endif()
    add_library(farbot INTERFACE)
    add_library(farbot::farbot ALIAS farbot)

    target_include_directories(farbot INTERFACE
            $<BUILD_INTERFACE:${farbot_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
endif()

add_executable(custom_queue_example
  customqueuemain.cpp
)

target_link_libraries(custom_queue_example
  PRIVATE
    rtlog::rtlog
    farbot::farbot
)
