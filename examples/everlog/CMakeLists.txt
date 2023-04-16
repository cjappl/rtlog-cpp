add_executable(everlog 
    everlogmain.cpp
)

target_link_libraries(everlog
    PRIVATE
        rtlog::rtlog
)
