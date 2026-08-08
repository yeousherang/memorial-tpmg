cmake_minimum_required(VERSION 3.25)

project(
    Memorial
    VERSION 0.1.0
    DESCRIPTION "Temporal Probabilistic Multilayer Graph library"
    LANGUAGES CXX
)

include(CTest)

option(MEMORIAL_BUILD_EXAMPLES "Build Memorial examples" ON)
option(MEMORIAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(MEMORIAL_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(MEMORIAL_FETCH_DEPENDENCIES "Fetch missing test dependencies" ON)

add_library(memorial_warnings INTERFACE)
add_library(memorial::warnings ALIAS memorial_warnings)

if(MSVC)
    target_compile_options(memorial_warnings INTERFACE /W4 /permissive-)
    if(MEMORIAL_WARNINGS_AS_ERRORS)
        target_compile_options(memorial_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(memorial_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(MEMORIAL_WARNINGS_AS_ERRORS)
        target_compile_options(memorial_warnings INTERFACE -Werror)
    endif()
endif()

add_library(memorial_sanitizers INTERFACE)
add_library(memorial::sanitizers ALIAS memorial_sanitizers)

if(MEMORIAL_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND NOT WIN32)
        target_compile_options(memorial_sanitizers INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(memorial_sanitizers INTERFACE -fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "MEMORIAL_ENABLE_SANITIZERS is unsupported by this toolchain")
    endif()
endif()

add_library(memorial STATIC src/runtime/library.cpp)
add_library(memorial::memorial ALIAS memorial)
target_compile_features(memorial PUBLIC cxx_std_23)
target_include_directories(
    memorial
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(memorial PRIVATE memorial::warnings memorial::sanitizers)
set_target_properties(memorial PROPERTIES CXX_EXTENSIONS OFF)

if(MEMORIAL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE MEMORIAL_FORMAT_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp"
    )
    add_custom_target(
        format
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${MEMORIAL_FORMAT_FILES}
        COMMENT "Formatting C++ sources"
        VERBATIM
    )
    add_custom_target(
        format-check
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${MEMORIAL_FORMAT_FILES}
        COMMENT "Checking C++ formatting"
        VERBATIM
    )
endif()
