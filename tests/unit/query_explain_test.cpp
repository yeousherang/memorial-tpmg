#include <memorial/query/explain.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string_view>

namespace {

using namespace std::chrono_literals;
using namespace memorial::query;

TEST(QueryExplain, ReportsOptimizedPlanColumnsCandidatesAndCardinality) {
    const auto valid =
        *memorial::valid_interval::make(memorial::timestamp{10ns}, memorial::timestamp{20ns});
    const auto transaction = *memorial::transaction_interval::open_ended(memorial::timestamp{30ns});
    const auto source = *memorial::provenance::make(memorial::provenance_kind::inferred, "model",
                                                    memorial::model_run_id{2U});
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto ghost = delta.nodes<memorial::domain::ghost_tag>().append(
        memorial::worldline_id{1U}, valid, transaction, source, 0.8F, 0.9F, 0.7);
    const auto action = delta.nodes<memorial::domain::action_tag>().append(
        memorial::worldline_id{1U}, valid, transaction, source, 0.9F);
    ASSERT_TRUE(ghost);
    ASSERT_TRUE(action);
    ASSERT_TRUE((delta.append_edge<memorial::domain::ghost_tag, memorial::domain::biases_relation,
                                   memorial::domain::action_tag>(*ghost, *action, 0.8, 0.6)));
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, memorial::worldline_id{1U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(delta));
    ASSERT_TRUE(graph);

    const auto query =
        from<memorial::memorial_schema, memorial::domain::ghost_node>() |
        where(prop<"activation"> > 0.5F) |
        traverse<memorial::domain::biases_relation, memorial::domain::action_node>() |
        project<"confidence">() | limit(4U);
    const auto plan = explain(*graph, query);

    ASSERT_TRUE(plan);
    EXPECT_EQ(plan->signature.size(), 18U);
    EXPECT_EQ(std::string_view{plan->signature}.substr(0U, 2U), "q-");
    EXPECT_EQ(plan->logical_ast.size(), 5U);
    EXPECT_EQ(plan->optimized_ast.size(), 5U);
    ASSERT_EQ(plan->required_columns.size(), 2U);
    EXPECT_EQ(plan->required_columns[0].key, "activation");
    EXPECT_EQ(plan->required_columns[1].key, "confidence");
    EXPECT_EQ(plan->estimated_cardinality, 1U);
    EXPECT_EQ(plan->estimated_traversal_cardinality, 1U);
    EXPECT_EQ(plan->actual_cardinality, 1U);

    const auto adjacency = std::ranges::find_if(plan->index_candidates, [](const auto& candidate) {
        return candidate.kind == index_kind::adjacency;
    });
    ASSERT_NE(adjacency, plan->index_candidates.end());
    EXPECT_TRUE(adjacency->available);
    EXPECT_TRUE(adjacency->selected);
    const auto valid_time = std::ranges::find_if(plan->index_candidates, [](const auto& candidate) {
        return candidate.kind == index_kind::valid_time;
    });
    ASSERT_NE(valid_time, plan->index_candidates.end());
    EXPECT_TRUE(valid_time->available);
    EXPECT_TRUE(valid_time->selected);
    const auto sparse_lookup =
        std::ranges::find_if(plan->kernel_candidates, [](const auto& candidate) {
            return candidate.kind == kernel_kind::sparse_index_lookup;
        });
    ASSERT_NE(sparse_lookup, plan->kernel_candidates.end());
    EXPECT_TRUE(sparse_lookup->selected);
    const auto property = std::ranges::find_if(plan->index_candidates, [](const auto& candidate) {
        return candidate.kind == index_kind::property;
    });
    ASSERT_NE(property, plan->index_candidates.end());
    EXPECT_TRUE(property->available);
    EXPECT_TRUE(property->selected);
    const auto traversal = std::ranges::find_if(plan->kernel_candidates, [](const auto& candidate) {
        return candidate.kind == kernel_kind::single_thread_traversal;
    });
    ASSERT_NE(traversal, plan->kernel_candidates.end());
    EXPECT_TRUE(traversal->selected);
}

TEST(QueryExplain, EstimatesTheIndexedSourceCandidateCount) {
    const auto active =
        *memorial::valid_interval::make(memorial::timestamp{10ns}, memorial::timestamp{20ns});
    const auto future =
        *memorial::valid_interval::make(memorial::timestamp{20ns}, memorial::timestamp{30ns});
    const auto transaction = *memorial::transaction_interval::open_ended(memorial::timestamp{5ns});
    const auto source = *memorial::provenance::make(memorial::provenance_kind::inferred, "model");
    memorial::delta_store<memorial::memorial_schema> delta;
    auto& thoughts = delta.nodes<memorial::domain::thought_tag>();
    ASSERT_TRUE(
        thoughts.append(memorial::worldline_id{1U}, active, transaction, source, 0.7F, 0.8F));
    ASSERT_TRUE(
        thoughts.append(memorial::worldline_id{2U}, active, transaction, source, 0.6F, 0.7F));
    ASSERT_TRUE(
        thoughts.append(memorial::worldline_id{1U}, future, transaction, source, 0.5F, 0.6F));
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, memorial::worldline_id{1U}, memorial::timestamp{15ns},
        memorial::timestamp{10ns}, std::move(delta));
    ASSERT_TRUE(graph);

    const auto plan =
        explain(*graph, from<memorial::memorial_schema, memorial::domain::thought_node>());

    ASSERT_TRUE(plan);
    EXPECT_EQ(graph->node_extent<memorial::domain::thought_tag>(), 3U);
    EXPECT_EQ(plan->estimated_cardinality, 1U);
    EXPECT_EQ(plan->actual_cardinality, 1U);
}

TEST(QueryExplain, UsesStableSignatureForTheSameCompiledQueryType) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, memorial::worldline_id{1U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(delta));
    ASSERT_TRUE(graph);

    const auto low = from<memorial::memorial_schema, memorial::domain::thought_node>() |
                     where(prop<"confidence"> > 0.2F);
    const auto high = from<memorial::memorial_schema, memorial::domain::thought_node>() |
                      where(prop<"confidence"> > 0.8F);
    const auto low_plan = explain(*graph, low);
    const auto high_plan = explain(*graph, high);
    ASSERT_TRUE(low_plan);
    ASSERT_TRUE(high_plan);
    EXPECT_EQ(low_plan->signature, high_plan->signature);
}

TEST(QueryExplain, PropagatesSelectorMismatch) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto graph = memorial::snapshot<memorial::memorial_schema>::publish(
        memorial::generation_id{3U}, memorial::worldline_id{1U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(delta));
    ASSERT_TRUE(graph);
    const auto query = from<memorial::memorial_schema, memorial::domain::thought_node>() |
                       in_worldline(memorial::worldline_id{9U});

    const auto plan = explain(*graph, query);
    ASSERT_FALSE(plan);
    EXPECT_EQ(plan.error().code(), memorial::graph_errc::worldline_mismatch);
}

} // namespace
