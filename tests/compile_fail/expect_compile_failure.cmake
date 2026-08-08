get_filename_component(OBJECT_DIR "${OBJECT}" DIRECTORY)
file(MAKE_DIRECTORY "${OBJECT_DIR}")

if(CXX_COMPILER_ID STREQUAL "MSVC")
    set(COMPILE_ARGUMENTS /nologo /std:c++17 /c "${SOURCE}" "/Fo${OBJECT}")
else()
    set(COMPILE_ARGUMENTS -std=c++17 -c "${SOURCE}" -o "${OBJECT}")
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
