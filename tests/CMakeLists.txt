include(FetchContent)

find_package(GTest 1.17 CONFIG QUIET)
if(NOT GTest_FOUND)
    if(NOT MEMORIAL_FETCH_DEPENDENCIES)
        message(FATAL_ERROR "GoogleTest 1.17 or newer is required when BUILD_TESTING is enabled")
    endif()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    if(MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(memorial_static_tests static/library_contracts.cpp)
target_link_libraries(memorial_static_tests PRIVATE memorial::memorial memorial::warnings memorial::sanitizers)
add_test(NAME static.library_contracts COMMAND memorial_static_tests)

add_executable(memorial_type_list_tests static/type_list.cpp)
target_link_libraries(
    memorial_type_list_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.type_list COMMAND memorial_type_list_tests)

add_executable(memorial_unit_tests unit/version_test.cpp)
target_link_libraries(
    memorial_unit_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers GTest::gtest_main
)
gtest_discover_tests(memorial_unit_tests TEST_PREFIX "unit.")

add_test(
    NAME compile_fail.unsupported_standard
    COMMAND
        "${CMAKE_COMMAND}"
        -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/unsupported_standard.cpp
        -DOBJECT=${CMAKE_CURRENT_BINARY_DIR}/compile_fail/unsupported_standard.o
        -P ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/expect_compile_failure.cmake
)
