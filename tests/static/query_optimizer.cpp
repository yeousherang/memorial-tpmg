#include <memorial/query/optimizer.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <chrono>
#include <concepts>

using namespace std::chrono_literals;
using namespace memorial::query;

constexpr auto original = from<memorial::memorial_schema, memorial::domain::thought_node>() |
                          active_at(memorial::timestamp{10ns}) | where(prop<"activation"> > 0.4F) |
                          where(prop<"confidence"> > 0.7F) | project<"confidence">() | limit(8U) |
                          limit(3U);
constexpr auto optimized = optimize(original);

using optimized_expression = typename decltype(optimized)::expression_type;
using expected_columns =
    memorial::meta::type_list<required_column<memorial::domain::thought_tag, "activation">,
                              required_column<memorial::domain::thought_tag, "confidence">>;

static_assert(Query<decltype(optimized)>);
static_assert(optimization_info<decltype(optimized)>::filter_count == 2U);
static_assert(optimization_info<decltype(optimized)>::traversal_count == 0U);
static_assert(std::same_as<typename optimization_info<decltype(optimized)>::required_columns,
                           expected_columns>);
static_assert(optimized.expression().operation.count == 3U);
static_assert(optimized.expression().previous.previous.operation.value ==
              memorial::timestamp{10ns});
static_assert(
    std::tuple_size_v<std::remove_cvref_t<
        decltype(optimized.expression().previous.previous.previous.operation.predicates)>> == 2U);

int main() {}
