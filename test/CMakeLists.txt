find_package(GTest REQUIRED)

add_executable(tests
    algorithm_tests.cpp
    allocated_arrays_tests.cpp
    minmax_heap_tests.cpp
    pack_manipulation_tests.cpp
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

