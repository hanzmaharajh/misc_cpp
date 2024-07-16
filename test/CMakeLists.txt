find_package(GTest CONFIG REQUIRED)

add_executable(tests
    algorithm_test.cpp
    allocated_storages_test.cpp
    array_of_optional_test.cpp
    comp_element_test.cpp
    dense_index_map_test.cpp
    log2_test.cpp
    minmax_heap_test.cpp
    pack_manipulation_test.cpp
    semaphore_test.cpp
    size_aware_cache_test.cpp
    vector_of_optional_test.cpp
)


target_link_libraries(tests
    PRIVATE
        project_options
        project_warnings
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        misc_lib
)

add_test(NAME unit_tests COMMAND tests)
