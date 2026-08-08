get_filename_component(OBJECT_DIR "${OBJECT}" DIRECTORY)
file(MAKE_DIRECTORY "${OBJECT_DIR}")

if(CXX_COMPILER_ID STREQUAL "MSVC")
    if(STANDARD STREQUAL "23")
        set(STANDARD_FLAG /std:c++latest)
    else()
        set(STANDARD_FLAG "/std:c++${STANDARD}")
    endif()
    set(COMPILE_ARGUMENTS /nologo ${STANDARD_FLAG} /c "${SOURCE}" "/Fo${OBJECT}")
    if(DEFINED INCLUDE_DIR)
        list(APPEND COMPILE_ARGUMENTS "/I${INCLUDE_DIR}")
    endif()
else()
    set(COMPILE_ARGUMENTS "-std=c++${STANDARD}" -c "${SOURCE}" -o "${OBJECT}")
    if(DEFINED INCLUDE_DIR)
        list(APPEND COMPILE_ARGUMENTS "-I${INCLUDE_DIR}")
    endif()
endif()

execute_process(
    COMMAND "${CXX_COMPILER}" ${COMPILE_ARGUMENTS}
    RESULT_VARIABLE COMPILE_RESULT
    OUTPUT_VARIABLE COMPILER_OUTPUT
    ERROR_VARIABLE COMPILER_ERROR
)

if(COMPILE_RESULT EQUAL 0)
    message(FATAL_ERROR "Expected compilation to fail, but it succeeded")
endif()

if(DEFINED EXPECTED_PATTERN)
    set(FULL_OUTPUT "${COMPILER_OUTPUT}\n${COMPILER_ERROR}")
    if(NOT FULL_OUTPUT MATCHES "${EXPECTED_PATTERN}")
        message(FATAL_ERROR "Compilation failed without expected diagnostic: ${EXPECTED_PATTERN}\n${FULL_OUTPUT}")
    endif()
endif()
