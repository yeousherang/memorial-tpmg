add_executable(memorial_minimal minimal.cpp)
target_link_libraries(memorial_minimal PRIVATE memorial::memorial memorial::warnings memorial::sanitizers)

