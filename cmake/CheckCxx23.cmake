include(CheckCXXSourceCompiles)

function(memorial_check_cxx23)
    check_cxx_source_compiles(
        [[
            #include <expected>

            int main() {
                const std::expected<int, int> result{42};
                return result.value() == 42 ? 0 : 1;
            }
        ]]
        MEMORIAL_HAS_STD_EXPECTED
    )

    if(NOT MEMORIAL_HAS_STD_EXPECTED)
        message(FATAL_ERROR
            "Memorial requires a C++23 standard library implementation of std::expected"
        )
    endif()

    check_cxx_source_compiles(
        [[
            #include <cstddef>

            template <std::size_t Extent>
            struct fixed_string {
                char value[Extent]{};

                consteval fixed_string(const char (&text)[Extent]) {
                    for (std::size_t index = 0; index < Extent; ++index) {
                        value[index] = text[index];
                    }
                }
            };

            template <fixed_string Name>
            struct named {};

            using compile_time_name = named<"memory">;

            int main() {
                return sizeof(compile_time_name) == 0;
            }
        ]]
        MEMORIAL_HAS_STRUCTURAL_NTTP
    )

    if(NOT MEMORIAL_HAS_STRUCTURAL_NTTP)
        message(FATAL_ERROR
            "Memorial requires compiler support for structural non-type template parameters"
        )
    endif()
endfunction()

