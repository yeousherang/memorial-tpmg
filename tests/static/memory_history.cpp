#include <memorial/runtime/memory_history.hpp>

#include <concepts>
#include <functional>
#include <string>

struct memory_tag {};

using history = memorial::memory_history<memorial::node_id<memory_tag>, std::string>;

static_assert(std::same_as<history::memory_id_type, memorial::node_id<memory_tag>>);
static_assert(std::same_as<history::value_type, std::string>);
static_assert(
    std::same_as<decltype(std::declval<const history&>().at(
                     std::declval<history::memory_id_type>(),
                     std::declval<memorial::perspective_id>(), std::declval<memorial::timestamp>(),
                     std::declval<memorial::timestamp>(), std::declval<memorial::worldline_id>())),
                 memorial::result<std::reference_wrapper<const history::event_type>>>);

int main() {}
