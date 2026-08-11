#include <memorial/query/executor.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <gtest/gtest.h>

#include <chrono>

namespace {

using namespace std::chrono_literals;
using namespace memorial::query;

struct query_fixture {
    memorial::worldline_id worldline{1U};
    memorial::timestamp valid_at{15ns};
    memorial::timestamp known_at{35ns};
    memorial::valid_interval valid =
        *memorial::valid_interval::make(memorial::timestamp{10ns}, memorial::timestamp{20ns});
    memorial::transaction_interval transaction =
        *memorial::transaction_interval::open_ended(memorial::timestamp{30ns});
    memorial::provenance source = *memorial::provenance::make(memorial::provenance_kind::inferred,
                                                              "model", memorial::model_run_id{2U});
};

TEST(QueryExecutor, FiltersTraversesProjectsRanksAndLimits) {
    const query_fixture fixture;
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto weak_ghost = delta.nodes<memorial::domain::ghost_tag>().append(
        fixture.worldline, fixture.valid, fixture.transaction, fixture.source, 0.4F, 0.9F, 0.5);
    const auto strong_ghost = delta.nodes<memorial::domain::ghost_tag>().append(
        fixture.worldline, fixture.valid, fixture.transaction, fixture.source, 0.8F, 0.9F, 0.8);
    const auto weaker_action = delta.nodes<memorial::domain::action_tag>().append(
        fixture.worldline, fixture.valid, fixture.transaction, fixture.source, 0.72F);
    const auto stronger_action = delta.nodes<memorial::domain::action_tag>().append(
        fixture.worldline, fixture.valid, fixture.transaction, fixture.source, 0.93F);
    ASSERT_TRUE(weak_ghost);
    ASSERT_TRUE(strong_ghost);
    ASSERT_TRUE(weaker_action);
    ASSERT_TRUE(stronger_action);
    ASSERT_TRUE(
        (delta.append_edge<memorial::domain::ghost_tag, memorial::domain::biases_relation,
                           memorial::domain::action_tag>(*weak_ghost, *stronger_action, 0.8, 0.5)));
    ASSERT_TRUE(
        (delta.append_edge<memorial::domain::ghost_tag, memorial::domain::biases_relation,
                           memorial::domain::action_tag>(*strong_ghost, *weaker_action, 0.7, 0.4)));
    ASSERT_TRUE((delta.append_edge<memorial::domain::ghost_tag, memorial::domain::biases_relation,
                                   memorial::domain::action_tag>(*strong_ghost, *stronger_action,
                                                                 0.9, 0.8)));
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, fixture.worldline, fixture.valid_at, fixture.known_at,
        std::move(delta));
    ASSERT_TRUE(graph);

    const auto query =
        from<memorial::memorial_schema, memorial::domain::ghost_node>() |
        active_at(fixture.valid_at) | known_at(fixture.known_at) | in_worldline(fixture.worldline) |
        where(prop<"activation"> > 0.5F) |
        traverse<memorial::domain::biases_relation, memorial::domain::action_node>() |
        where(prop<"confidence"> > 0.7F) | project<"confidence">() |
        top_k<2>(by<"confidence">.descending()) | limit(1U);
    const auto result = execute(*graph, query);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ(result->front().id, *stronger_action);
    EXPECT_FLOAT_EQ(result->front().value<"confidence">(), 0.93F);
}

TEST(QueryExecutor, ReturnsOwningIdsWithoutProjection) {
    const query_fixture fixture;
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto thought = delta.nodes<memorial::domain::thought_tag>().append(
        fixture.worldline, fixture.valid, fixture.transaction, fixture.source, 0.8F, 0.9F);
    ASSERT_TRUE(thought);
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, fixture.worldline, fixture.valid_at, fixture.known_at,
        std::move(delta));
    ASSERT_TRUE(graph);

    const auto result = execute(
        *graph, from<memorial::memorial_schema, memorial::domain::thought_tag>() | limit(10U));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->size(), 1U);
    EXPECT_EQ(result->front(), *thought);
}

TEST(QueryExecutor, RejectsSnapshotSelectorMismatch) {
    const query_fixture fixture;
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, fixture.worldline, fixture.valid_at, fixture.known_at,
        std::move(delta));
    ASSERT_TRUE(graph);

    const auto query = from<memorial::memorial_schema, memorial::domain::thought_tag>() |
                       in_worldline(memorial::worldline_id{9U});
    const auto result = execute(*graph, query);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), memorial::graph_errc::worldline_mismatch);
}

TEST(QueryExecutor, CostModelChoosesOnlySelectivePropertyLookups) {
    EXPECT_TRUE(detail::should_use_property_index(2U, 10U));
    EXPECT_FALSE(detail::should_use_property_index(8U, 10U));
    EXPECT_FALSE(detail::should_use_property_index(10U, 10U));
}

} // namespace
