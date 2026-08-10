#include <memorial/runtime/worldline.hpp>

#include <concepts>
#include <functional>

static_assert(memorial::StrongId<memorial::intervention_id>);
static_assert(
    std::same_as<decltype(std::declval<const memorial::worldline_registry&>().find(
                     std::declval<memorial::worldline_id>())),
                 memorial::result<std::reference_wrapper<const memorial::worldline_record>>>);
static_assert(std::copy_constructible<memorial::worldline_record>);

int main() {}
