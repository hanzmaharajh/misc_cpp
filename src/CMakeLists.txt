set(MISC_CPP_FILES "")

if (MISC_ENABLE_HEADER_ONLY)
    target_compile_definitions(misc_lib MISC_HEADER_ONLY)
else()
    list(APPEND MISC_CPP_FILES
        misc/file.cpp
    )
endif()


add_library(misc_lib
    ${MISC_CPP_FILES}
)

target_include_directories(misc_lib
    INTERFACE 
        .
)

target_link_libraries(misc_lib
    PUBLIC
        project_options
        project_warnings
)
