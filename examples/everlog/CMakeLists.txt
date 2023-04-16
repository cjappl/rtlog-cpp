add_executable(everlog 
    everlogmain.cpp
)

target_link_libraries(everlog
    PRIVATE
        rtlog::rtlog
)

target_compile_definitions(everlog
    PRIVATE
        $<$<BOOL:${RTLOG_HAS_PTHREADS}>:RTLOG_HAS_PTHREADS>
)
