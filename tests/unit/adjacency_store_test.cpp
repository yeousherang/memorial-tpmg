#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/delta_store.hpp>

#include <gtest/gtest.h>

#include <chrono>

namespace {

using namespace std::chrono_literals;

struct row_context {
    memorial::worldline_id worldline{1U};
    memorial::valid_interval valid =
        *memorial::valid_interval::make(memorial::timestamp{10ns}, memorial::timestamp{20ns});
    memorial::transaction_interval transaction =
        *memorial::transaction_interval::open_ended(memorial::timestamp{30ns});
    memorial::provenance source =
        *memorial::provenance::make(memorial::provenance_kind::observed, "journal");
};

TEST(AdjacencyStore, AppendsTypedEdgesAndBuildsBothDirections) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto experience = delta.nodes<memorial::domain::experience_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.9F);
    const auto perception = delta.nodes<memorial::domain::perception_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.8F);
    ASSERT_TRUE(experience);
    ASSERT_TRUE(perception);

    const auto edge =
        delta.append_edge<memorial::domain::experience_tag, memorial::domain::generates_relation,
                          memorial::domain::perception_tag>(*experience, *perception, 0.75);

    ASSERT_TRUE(edge);
    const auto& edges =
        delta.edges<memorial::domain::experience_tag, memorial::domain::generates_relation,
                    memorial::domain::perception_tag>();
    EXPECT_EQ(edges.size(), 1U);
    ASSERT_EQ(edges.outgoing(*experience).size(), 1U);
    ASSERT_EQ(edges.incoming(*perception).size(), 1U);
    EXPECT_EQ(edges.outgoing(*experience).front(), *edge);
    EXPECT_EQ(edges.incoming(*perception).front(), *edge);
    ASSERT_TRUE(edges.property<"existence_probability">(*edge));
    EXPECT_DOUBLE_EQ(edges.property<"existence_probability">(*edge)->get(), 0.75);
    EXPECT_EQ(*edges.source(*edge), *experience);
    EXPECT_EQ(*edges.target(*edge), *perception);
}

TEST(AdjacencyStore, KeepsRelationsInIndependentStores) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto first = delta.nodes<memorial::domain::experience_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.9F);
    const auto second = delta.nodes<memorial::domain::experience_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.8F);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    ASSERT_TRUE((delta.append_edge<memorial::domain::experience_tag,
                                   memorial::domain::temporal_next_relation,
                                   memorial::domain::experience_tag>(*first, *second, 1.0)));

    EXPECT_EQ(
        (delta
             .edges<memorial::domain::experience_tag, memorial::domain::temporal_next_relation,
                    memorial::domain::experience_tag>()
             .size()),
        1U);
    EXPECT_TRUE((delta
                     .edges<memorial::domain::experience_tag, memorial::domain::generates_relation,
                            memorial::domain::perception_tag>()
                     .empty()));
}

TEST(AdjacencyStore, RejectsEndpointsMissingFromNodeStores) {
    memorial::delta_store<memorial::memorial_schema> delta;

    const auto edge =
        delta.append_edge<memorial::domain::experience_tag, memorial::domain::generates_relation,
                          memorial::domain::perception_tag>(
            memorial::node_id<memorial::domain::experience_tag>{0U},
            memorial::node_id<memorial::domain::perception_tag>{0U}, 0.5);

    ASSERT_FALSE(edge);
    EXPECT_EQ(edge.error().code(), memorial::graph_errc::id_not_found);
}

TEST(AdjacencyStore, RejectsEndpointsFromDifferentWorldlines) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto experience = delta.nodes<memorial::domain::experience_tag>().append(
        memorial::worldline_id{1U}, context.valid, context.transaction, context.source, 0.9F);
    const auto perception = delta.nodes<memorial::domain::perception_tag>().append(
        memorial::worldline_id{2U}, context.valid, context.transaction, context.source, 0.8F);
    ASSERT_TRUE(experience);
    ASSERT_TRUE(perception);

    const auto edge =
        delta.append_edge<memorial::domain::experience_tag, memorial::domain::generates_relation,
                          memorial::domain::perception_tag>(*experience, *perception, 0.5);

    ASSERT_FALSE(edge);
    EXPECT_EQ(edge.error().code(), memorial::graph_errc::worldline_mismatch);
}

} // namespace
