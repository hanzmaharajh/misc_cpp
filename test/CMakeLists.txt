find_package(GTest CONFIG REQUIRED)

add_executable(tests
    algorithm_tests.cpp
    allocated_arrays_tests.cpp
    minmax_heap_tests.cpp
    pack_manipulation_tests.cpp
    semaphore_tests.cpp
    size_aware_cache_test.cpp
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
