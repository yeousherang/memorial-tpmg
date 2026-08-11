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

file(
    GLOB_RECURSE MEMORIAL_PUBLIC_HEADERS
    CONFIGURE_DEPENDS
    RELATIVE "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/memorial/*.hpp"
)
set(MEMORIAL_HEADER_TEST_SOURCES)
foreach(HEADER IN LISTS MEMORIAL_PUBLIC_HEADERS)
    string(MAKE_C_IDENTIFIER "${HEADER}" HEADER_ID)
    set(HEADER_SOURCE
        "${CMAKE_CURRENT_BINARY_DIR}/header_self_containment/${HEADER_ID}.cpp"
    )
    file(
        GENERATE
        OUTPUT "${HEADER_SOURCE}"
        CONTENT "#include <${HEADER}>\n\nvoid memorial_header_${HEADER_ID}() {}\n"
    )
    list(APPEND MEMORIAL_HEADER_TEST_SOURCES "${HEADER_SOURCE}")
endforeach()

add_executable(
    memorial_header_self_containment_tests
    static/header_self_containment_main.cpp
    ${MEMORIAL_HEADER_TEST_SOURCES}
)
target_link_libraries(
    memorial_header_self_containment_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.header_self_containment COMMAND memorial_header_self_containment_tests)

add_executable(memorial_static_tests static/library_contracts.cpp)
target_link_libraries(memorial_static_tests PRIVATE memorial::memorial memorial::warnings memorial::sanitizers)
add_test(NAME static.library_contracts COMMAND memorial_static_tests)

add_executable(memorial_type_list_tests static/type_list.cpp)
target_link_libraries(
    memorial_type_list_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.type_list COMMAND memorial_type_list_tests)

add_executable(memorial_fixed_string_tests static/fixed_string.cpp)
target_link_libraries(
    memorial_fixed_string_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.fixed_string COMMAND memorial_fixed_string_tests)

add_executable(memorial_strong_id_tests static/strong_id.cpp)
target_link_libraries(
    memorial_strong_id_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.strong_id COMMAND memorial_strong_id_tests)

add_executable(memorial_type_map_tests static/type_map.cpp)
target_link_libraries(
    memorial_type_map_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.type_map COMMAND memorial_type_map_tests)

add_executable(memorial_property_tests static/property.cpp)
target_link_libraries(
    memorial_property_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.property COMMAND memorial_property_tests)

add_executable(memorial_node_tests static/node.cpp)
target_link_libraries(
    memorial_node_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.node COMMAND memorial_node_tests)

add_executable(memorial_edge_tests static/edge.cpp)
target_link_libraries(
    memorial_edge_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.edge COMMAND memorial_edge_tests)

add_executable(memorial_graph_schema_tests static/graph_schema.cpp)
target_link_libraries(
    memorial_graph_schema_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.graph_schema COMMAND memorial_graph_schema_tests)

add_executable(memorial_relation_rules_tests static/relation_rules.cpp)
target_link_libraries(
    memorial_relation_rules_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.relation_rules COMMAND memorial_relation_rules_tests)

add_executable(memorial_domain_schema_tests static/memorial_schema.cpp)
target_link_libraries(
    memorial_domain_schema_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.memorial_schema COMMAND memorial_domain_schema_tests)

add_executable(memorial_node_store_tests static/node_store.cpp)
target_link_libraries(
    memorial_node_store_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.node_store COMMAND memorial_node_store_tests)

add_executable(memorial_runtime_types_tests static/runtime_types.cpp)
target_link_libraries(
    memorial_runtime_types_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.runtime_types COMMAND memorial_runtime_types_tests)

add_executable(memorial_delta_store_tests static/delta_store.cpp)
target_link_libraries(
    memorial_delta_store_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.delta_store COMMAND memorial_delta_store_tests)

add_executable(memorial_adjacency_store_tests static/adjacency_store.cpp)
target_link_libraries(
    memorial_adjacency_store_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.adjacency_store COMMAND memorial_adjacency_store_tests)

add_executable(memorial_snapshot_tests static/snapshot.cpp)
target_link_libraries(
    memorial_snapshot_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.snapshot COMMAND memorial_snapshot_tests)

add_executable(memorial_event_log_tests static/event_log.cpp)
target_link_libraries(
    memorial_event_log_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.event_log COMMAND memorial_event_log_tests)

add_executable(memorial_replay_tests static/replay.cpp)
target_link_libraries(
    memorial_replay_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.replay COMMAND memorial_replay_tests)

add_executable(memorial_canonical_hash_tests static/canonical_hash.cpp)
target_link_libraries(
    memorial_canonical_hash_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.canonical_hash COMMAND memorial_canonical_hash_tests)

add_executable(memorial_memory_history_tests static/memory_history.cpp)
target_link_libraries(
    memorial_memory_history_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.memory_history COMMAND memorial_memory_history_tests)

add_executable(memorial_worldline_tests static/worldline.cpp)
target_link_libraries(
    memorial_worldline_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.worldline COMMAND memorial_worldline_tests)

add_executable(memorial_query_dsl_tests static/query_dsl.cpp)
target_link_libraries(
    memorial_query_dsl_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.query_dsl COMMAND memorial_query_dsl_tests)

add_executable(memorial_query_executor_tests static/query_executor.cpp)
target_link_libraries(
    memorial_query_executor_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.query_executor COMMAND memorial_query_executor_tests)

add_executable(memorial_query_optimizer_tests static/query_optimizer.cpp)
target_link_libraries(
    memorial_query_optimizer_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.query_optimizer COMMAND memorial_query_optimizer_tests)

add_executable(memorial_query_explain_tests static/query_explain.cpp)
target_link_libraries(
    memorial_query_explain_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers
)
add_test(NAME static.query_explain COMMAND memorial_query_explain_tests)

add_executable(
    memorial_unit_tests
    unit/adjacency_store_test.cpp
    unit/canonical_hash_test.cpp
    unit/delta_store_test.cpp
    unit/event_log_test.cpp
    unit/memory_history_test.cpp
    unit/node_store_test.cpp
    unit/query_dsl_test.cpp
    unit/query_executor_test.cpp
    unit/query_explain_test.cpp
    unit/query_optimizer_test.cpp
    unit/replay_test.cpp
    unit/runtime_types_test.cpp
    unit/snapshot_test.cpp
    unit/strong_id_test.cpp
    unit/version_test.cpp
    unit/worldline_test.cpp
)
target_link_libraries(
    memorial_unit_tests
    PRIVATE memorial::memorial memorial::warnings memorial::sanitizers GTest::gtest_main
)
gtest_discover_tests(memorial_unit_tests TEST_PREFIX "unit.")

function(memorial_add_compile_fail_test NAME SOURCE STANDARD)
    set(OBJECT "${CMAKE_CURRENT_BINARY_DIR}/compile_fail/${NAME}.o")
    set(COMMAND_ARGS
        -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/${SOURCE}
        -DOBJECT=${OBJECT}
        -DSTANDARD=${STANDARD}
        -DINCLUDE_DIR=${PROJECT_SOURCE_DIR}/include
    )
    if(ARGC GREATER 3)
        list(APPEND COMMAND_ARGS -DEXPECTED_PATTERN=${ARGV3})
    endif()
    add_test(
        NAME compile_fail.${NAME}
        COMMAND "${CMAKE_COMMAND}" ${COMMAND_ARGS}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/expect_compile_failure.cmake
    )
endfunction()

memorial_add_compile_fail_test(unsupported_standard unsupported_standard.cpp 17)
memorial_add_compile_fail_test(
    duplicate_type_map_key
    duplicate_type_map_key.cpp
    23
    "type_map keys must be unique"
)
memorial_add_compile_fail_test(missing_type_map_key missing_type_map_key.cpp 23)
memorial_add_compile_fail_test(
    empty_property_key
    empty_property_key.cpp
    23
    "property key must not be empty"
)
memorial_add_compile_fail_test(invalid_property_value invalid_property_value.cpp 23)
memorial_add_compile_fail_test(
    duplicate_node_property
    duplicate_node_property.cpp
    23
    "node property keys must be unique"
)
memorial_add_compile_fail_test(invalid_node_property invalid_node_property.cpp 23)
memorial_add_compile_fail_test(missing_node_property missing_node_property.cpp 23)
memorial_add_compile_fail_test(missing_node_store_property missing_node_store_property.cpp 23)
memorial_add_compile_fail_test(unknown_delta_node unknown_delta_node.cpp 23)
memorial_add_compile_fail_test(unknown_delta_edge unknown_delta_edge.cpp 23)
memorial_add_compile_fail_test(wrong_edge_endpoint_id wrong_edge_endpoint_id.cpp 23)
memorial_add_compile_fail_test(
    duplicate_edge_property
    duplicate_edge_property.cpp
    23
    "edge property keys must be unique"
)
memorial_add_compile_fail_test(invalid_edge_property invalid_edge_property.cpp 23)
memorial_add_compile_fail_test(missing_edge_property missing_edge_property.cpp 23)
memorial_add_compile_fail_test(
    duplicate_schema_node
    duplicate_schema_node.cpp
    23
    "graph_schema node tags must be unique"
)
memorial_add_compile_fail_test(
    unknown_edge_endpoint
    unknown_edge_endpoint.cpp
    23
    "graph_schema edge endpoints must reference declared node tags"
)
memorial_add_compile_fail_test(
    duplicate_schema_edge
    duplicate_schema_edge.cpp
    23
    "graph_schema edge signatures must be unique"
)
memorial_add_compile_fail_test(
    invalid_layer_relation
    invalid_layer_relation.cpp
    23
    "graph_schema edge violates its relation layer rules"
)
memorial_add_compile_fail_test(
    duplicate_relation_rule
    duplicate_relation_rule.cpp
    23
    "relation rule signatures must be unique"
)
memorial_add_compile_fail_test(
    query_unknown_property
    query_unknown_property.cpp
    23
    "query predicate property is unavailable"
)
memorial_add_compile_fail_test(
    query_invalid_traversal
    query_invalid_traversal.cpp
    23
    "query traversal relation is not valid"
)
memorial_add_compile_fail_test(
    query_filtered_after_projection
    query_filtered_after_projection.cpp
    23
    "query predicate property is unavailable"
)
