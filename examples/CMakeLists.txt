add_executable(memorial_minimal minimal.cpp)
target_link_libraries(memorial_minimal PRIVATE memorial::memorial memorial::warnings memorial::sanitizers)

add_executable(memorial_schema_example schema.cpp)
target_link_libraries(
    memorial_schema_example
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)

add_executable(memorial_decision_episode decision_episode.cpp)
target_link_libraries(
    memorial_decision_episode
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)

if(BUILD_TESTING)
    add_test(NAME integration.decision_episode COMMAND memorial_decision_episode)
    set_tests_properties(
        integration.decision_episode
        PROPERTIES PASS_REGULAR_EXPRESSION
                   "Decision episode: 1 selected action, outcome confidence 0.78"
    )
endif()
