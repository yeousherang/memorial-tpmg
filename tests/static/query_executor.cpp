#include <memorial/query/executor.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
#include <vector>

using namespace memorial::query;

using snapshot_type = memorial::snapshot<memorial::memorial_schema>;
using query_type = decltype(from<memorial::memorial_schema, memorial::domain::thought_node>() |
                            project<"confidence">());
using row_type = projected_row<memorial::domain::thought_node,
                               memorial::meta::type_list<property_key_token<"confidence">>>;

static_assert(std::same_as<decltype(execute(std::declval<const snapshot_type&>(),
                                            std::declval<const query_type&>())),
                           memorial::result<std::vector<row_type>>>);
static_assert(
    std::same_as<decltype(std::declval<const row_type&>().value<"confidence">()), const float&>);

int main() {}
