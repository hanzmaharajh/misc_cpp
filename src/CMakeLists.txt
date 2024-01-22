add_library(misc_lib INTERFACE)

target_include_directories(misc_lib
    INTERFACE 
        .
)

target_link_libraries(misc_lib
    INTERFACE
        project_options
        project_warnings
)
