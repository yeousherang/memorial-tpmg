add_executable(memorial_static_tests static/library_contracts.cpp)
target_link_libraries(memorial_static_tests PRIVATE memorial::memorial memorial::warnings memorial::sanitizers)
add_test(NAME static.library_contracts COMMAND memorial_static_tests)

add_executable(memorial_unit_tests unit/version_test.cpp)
target_link_libraries(memorial_unit_tests PRIVATE memorial::memorial memorial::warnings memorial::sanitizers)
add_test(NAME unit.version COMMAND memorial_unit_tests)

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
