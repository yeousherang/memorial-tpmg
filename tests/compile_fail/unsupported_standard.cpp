#if __cplusplus >= 202302L
#error "This compile-fail fixture must be compiled as pre-C++23"
#endif

#include <expected>

int main() {
    std::expected<int, int> result{42};
    return result.value();
}
