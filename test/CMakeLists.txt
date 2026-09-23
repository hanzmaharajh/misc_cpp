find_package(GTest CONFIG REQUIRED)

set(MISC_TEST_CPP_FILES
    algorithm_test.cpp
    allocated_storages_test.cpp
    array_of_optional_test.cpp
    comp_element_test.cpp
    compact_int_array_test.cpp
    component_table_test.cpp
    dense_index_map_test.cpp
    iterator_test.cpp
    log2_test.cpp
    minmax_heap_test.cpp
    pack_manipulation_test.cpp
    semaphore_test.cpp
    size_aware_cache_test.cpp
    tagged_ptr_test.cpp
    test.cpp
    vector_of_optional_test.cpp
    variant_array_test.cpp
)

if (NOT MISC_ENABLE_HEADER_ONLY)
    list(APPEND MISC_TEST_CPP_FILES
        file_test.cpp
    )
endif()

add_executable(tests ${MISC_TEST_CPP_FILES})

target_include_directories(tests
    INTERFACE 
        .
)
target_link_libraries(tests
    PUBLIC
        project_options
        project_warnings
    PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        misc_lib
)

add_test(NAME unit_tests COMMAND tests)
