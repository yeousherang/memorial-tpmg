#include <memorial/query/executor.hpp>
#include <memorial/query/optimizer.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <gtest/gtest.h>

#include <chrono>

namespace {

using namespace std::chrono_literals;
using namespace memorial::query;

TEST(QueryOptimizer, OptimizedAndOriginalQueriesProduceEqualResults) {
    const auto valid =
        *memorial::valid_interval::make(memorial::timestamp{10ns}, memorial::timestamp{20ns});
    const auto transaction = *memorial::transaction_interval::open_ended(memorial::timestamp{30ns});
    const auto source = *memorial::provenance::make(memorial::provenance_kind::observed, "journal");
    memorial::delta_store<memorial::memorial_schema> delta;
    ASSERT_TRUE(delta.nodes<memorial::domain::thought_tag>().append(
        memorial::worldline_id{1U}, valid, transaction, source, 0.8F, 0.9F));
    ASSERT_TRUE(delta.nodes<memorial::domain::thought_tag>().append(
        memorial::worldline_id{1U}, valid, transaction, source, 0.6F, 0.5F));
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, memorial::worldline_id{1U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(delta));
    ASSERT_TRUE(graph);

    const auto query = from<memorial::memorial_schema, memorial::domain::thought_node>() |
                       active_at(memorial::timestamp{15ns}) | where(prop<"activation"> > 0.7F) |
                       where(prop<"confidence"> > 0.8F) | project<"confidence">() | limit(5U) |
                       limit(2U);
    const auto original_result = execute(*graph, query);
    const auto optimized_result = execute(*graph, optimize(query));

    ASSERT_TRUE(original_result);
    ASSERT_TRUE(optimized_result);
    ASSERT_EQ(original_result->size(), 1U);
    ASSERT_EQ(optimized_result->size(), original_result->size());
    EXPECT_EQ(optimized_result->front().id, original_result->front().id);
    EXPECT_EQ(optimized_result->front().value<"confidence">(),
              original_result->front().value<"confidence">());
}

TEST(QueryOptimizer, KeepsDifferentRuntimeThresholdsDuringFilterFusion) {
    const auto query = from<memorial::memorial_schema, memorial::domain::thought_node>() |
                       where(prop<"confidence"> > 0.4F) | where(prop<"confidence"> > 0.8F);
    const auto optimized = optimize(query);
    const auto& predicates = optimized.expression().operation.predicates;

    EXPECT_FLOAT_EQ(std::get<0>(predicates).value, 0.4F);
    EXPECT_FLOAT_EQ(std::get<1>(predicates).value, 0.8F);
}

} // namespace
