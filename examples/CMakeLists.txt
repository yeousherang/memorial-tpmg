add_executable(memorial_minimal minimal.cpp)
target_link_libraries(memorial_minimal PRIVATE memorial::memorial memorial::warnings memorial::sanitizers)

add_executable(memorial_schema_example schema.cpp)
target_link_libraries(
    memorial_schema_example
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
