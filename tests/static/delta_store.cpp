#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/delta_store.hpp>

#include <concepts>
#include <functional>
#include <vector>

using store = memorial::delta_store<memorial::memorial_schema>;
using thought_store = memorial::delta_node_store<memorial::domain::thought_node>;

static_assert(std::same_as<decltype(std::declval<store&>().nodes<memorial::domain::thought_tag>()),
                           thought_store&>);
static_assert(std::same_as<decltype(std::declval<const thought_store&>().property<"activation">(
                               std::declval<thought_store::id_type>())),
                           memorial::result<std::reference_wrapper<const float>>>);
static_assert(
    std::same_as<decltype(std::declval<const thought_store&>().indexed_candidates(
                     std::declval<memorial::worldline_id>(), std::declval<memorial::timestamp>(),
                     std::declval<memorial::timestamp>())),
                 memorial::result<std::vector<thought_store::id_type>>>);

int main() {}
