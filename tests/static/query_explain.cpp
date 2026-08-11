#include <memorial/query/explain.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <concepts>
#include <type_traits>

using namespace memorial::query;

using snapshot_type = memorial::snapshot<memorial::memorial_schema>;
using query_type = decltype(from<memorial::memorial_schema, memorial::domain::thought_node>() |
                            where(prop<"confidence"> > 0.5F) | project<"confidence">());

static_assert(std::same_as<decltype(explain(std::declval<const snapshot_type&>(),
                                            std::declval<const query_type&>())),
                           memorial::result<query_plan>>);
static_assert(std::same_as<decltype(query_plan::estimated_cardinality), std::size_t>);

int main() {}
