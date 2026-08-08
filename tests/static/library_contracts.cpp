#include <memorial/graph.hpp>

#include <concepts>
#include <string_view>

static_assert(std::same_as<decltype(memorial::version()), std::string_view>);
static_assert(noexcept(memorial::version()));
static_assert(memorial::version() == "0.1.0");

int main() {}
