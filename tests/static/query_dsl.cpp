#include <memorial/query/dsl.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <chrono>
#include <concepts>

using namespace std::chrono_literals;
using namespace memorial::query;

constexpr auto query =
    from<memorial::memorial_schema, memorial::domain::ghost_node>() |
    active_at(memorial::timestamp{10ns}) | in_worldline(memorial::worldline_id{0U}) |
    where(prop<"activation"> > 0.6F) |
    traverse<memorial::domain::biases_relation, memorial::domain::action_node>() |
    where(prop<"confidence"> >= 0.7F) | project<"confidence">() |
    top_k<10>(by<"confidence">.descending()) | limit(5U);

static_assert(Query<decltype(query)>);
static_assert(std::same_as<typename decltype(query)::schema_type, memorial::memorial_schema>);
static_assert(std::same_as<typename decltype(query)::current_node, memorial::domain::action_node>);
static_assert(std::same_as<typename decltype(query)::projection,
                           memorial::meta::type_list<property_key_token<"confidence">>>);
static_assert(query.expression().operation.count == 5U);

int main() {}
