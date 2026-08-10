#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/snapshot.hpp>

#include <gtest/gtest.h>

#include <chrono>

namespace {

using namespace std::chrono_literals;
using snapshot_type = memorial::snapshot<memorial::memorial_schema>;

struct row_context {
    memorial::worldline_id worldline{1U};
    memorial::valid_interval valid =
        *memorial::valid_interval::make(memorial::timestamp{10ns}, memorial::timestamp{20ns});
    memorial::transaction_interval transaction =
        *memorial::transaction_interval::open_ended(memorial::timestamp{30ns});
    memorial::provenance source =
        *memorial::provenance::make(memorial::provenance_kind::observed, "journal");
};

TEST(Snapshot, PublishesMovedDeltaAsAnImmutableGeneration) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto thought = delta.nodes<memorial::domain::thought_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.7F, 0.8F);
    ASSERT_TRUE(thought);

    auto published = snapshot_type::publish(memorial::generation_id{3U}, context.worldline,
                                            memorial::timestamp{15ns}, memorial::timestamp{35ns},
                                            std::move(delta));

    ASSERT_TRUE(published);
    EXPECT_EQ(published->generation(), memorial::generation_id{3U});
    ASSERT_TRUE((published->property<memorial::domain::thought_tag, "confidence">(*thought)));
    EXPECT_FLOAT_EQ(
        (published->property<memorial::domain::thought_tag, "confidence">(*thought))->get(), 0.8F);
}

TEST(Snapshot, AppliesValidAndTransactionTimeSelection) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto thought = delta.nodes<memorial::domain::thought_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.7F, 0.8F);
    ASSERT_TRUE(thought);

    auto published = snapshot_type::publish(memorial::generation_id{3U}, context.worldline,
                                            memorial::timestamp{20ns}, memorial::timestamp{35ns},
                                            std::move(delta));

    ASSERT_TRUE(published);
    const auto visible = published->contains(*thought);
    ASSERT_TRUE(visible);
    EXPECT_FALSE(*visible);
    const auto property =
        published->property<memorial::domain::thought_tag, "confidence">(*thought);
    ASSERT_FALSE(property);
    EXPECT_EQ(property.error().code(), memorial::graph_errc::id_not_found);
}

TEST(Snapshot, RejectsDirectLookupFromAnotherWorldline) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto thought = delta.nodes<memorial::domain::thought_tag>().append(
        memorial::worldline_id{2U}, context.valid, context.transaction, context.source, 0.7F, 0.8F);
    ASSERT_TRUE(thought);

    auto published = snapshot_type::publish(memorial::generation_id{3U}, context.worldline,
                                            memorial::timestamp{15ns}, memorial::timestamp{35ns},
                                            std::move(delta));
    ASSERT_TRUE(published);

    const auto property =
        published->property<memorial::domain::thought_tag, "confidence">(*thought);
    ASSERT_FALSE(property);
    EXPECT_EQ(property.error().code(), memorial::graph_errc::worldline_mismatch);
}

TEST(Snapshot, CopiesKeepThePublishedGenerationAlive) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto thought = delta.nodes<memorial::domain::thought_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.7F, 0.8F);
    ASSERT_TRUE(thought);
    auto published = snapshot_type::publish(memorial::generation_id{3U}, context.worldline,
                                            memorial::timestamp{15ns}, memorial::timestamp{35ns},
                                            std::move(delta));
    ASSERT_TRUE(published);

    const auto retained = *published;
    published = std::unexpected(memorial::graph_error{memorial::graph_errc::conflict});

    ASSERT_TRUE((retained.property<memorial::domain::thought_tag, "activation">(*thought)));
    EXPECT_FLOAT_EQ(
        (retained.property<memorial::domain::thought_tag, "activation">(*thought))->get(), 0.7F);
}

TEST(Snapshot, TraversalOmitsEdgesWhoseOtherEndpointIsInactive) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;
    const auto experience = delta.nodes<memorial::domain::experience_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.9F);
    const auto later =
        *memorial::valid_interval::make(memorial::timestamp{40ns}, memorial::timestamp{50ns});
    const auto perception = delta.nodes<memorial::domain::perception_tag>().append(
        context.worldline, later, context.transaction, context.source, 0.8F);
    ASSERT_TRUE(experience);
    ASSERT_TRUE(perception);
    ASSERT_TRUE(
        (delta.append_edge<memorial::domain::experience_tag, memorial::domain::generates_relation,
                           memorial::domain::perception_tag>(*experience, *perception, 0.75)));

    auto published = snapshot_type::publish(memorial::generation_id{3U}, context.worldline,
                                            memorial::timestamp{15ns}, memorial::timestamp{35ns},
                                            std::move(delta));
    ASSERT_TRUE(published);
    const auto outgoing =
        published->outgoing<memorial::domain::experience_tag, memorial::domain::generates_relation,
                            memorial::domain::perception_tag>(*experience);

    ASSERT_TRUE(outgoing);
    EXPECT_TRUE(outgoing->empty());
}

TEST(Snapshot, RejectsInvalidPublicationIds) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto published = snapshot_type::publish(
        memorial::generation_id::invalid(), memorial::worldline_id{1U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(delta));

    ASSERT_FALSE(published);
    EXPECT_EQ(published.error().code(), memorial::graph_errc::invalid_id);
}

TEST(Snapshot, BranchSharesParentRowsAndAllocatesDistinctLocalIds) {
    memorial::delta_store<memorial::memorial_schema> root_delta;
    const row_context root_context;
    const auto inherited = root_delta.nodes<memorial::domain::thought_tag>().append(
        root_context.worldline, root_context.valid, root_context.transaction, root_context.source,
        0.7F, 0.8F);
    ASSERT_TRUE(inherited);
    const auto root = snapshot_type::publish(memorial::generation_id{3U}, root_context.worldline,
                                             memorial::timestamp{15ns}, memorial::timestamp{35ns},
                                             std::move(root_delta));
    ASSERT_TRUE(root);

    auto branch_delta = root->make_branch_delta();
    const auto simulated = *memorial::provenance::make(
        memorial::provenance_kind::simulated, "counterfactual", memorial::model_run_id{4U});
    const auto local = branch_delta.nodes<memorial::domain::thought_tag>().append(
        memorial::worldline_id{2U}, root_context.valid, root_context.transaction, simulated, 0.9F,
        0.95F);
    ASSERT_TRUE(local);
    EXPECT_NE(*local, *inherited);

    const auto branch = snapshot_type::publish_branch(
        *root, memorial::generation_id{4U}, memorial::worldline_id{2U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(branch_delta));
    ASSERT_TRUE(branch);
    EXPECT_TRUE(branch->is_branch());
    ASSERT_NE(branch->parent(), nullptr);
    EXPECT_EQ(branch->parent()->worldline(), root->worldline());
    ASSERT_TRUE((branch->property<memorial::domain::thought_tag, "confidence">(*inherited)));
    EXPECT_FLOAT_EQ(
        (branch->property<memorial::domain::thought_tag, "confidence">(*inherited))->get(), 0.8F);
    const auto local_confidence =
        branch->property<memorial::domain::thought_tag, "confidence">(*local);
    ASSERT_TRUE(local_confidence) << local_confidence.error().detail();
    EXPECT_FLOAT_EQ(local_confidence->get(), 0.95F);

    EXPECT_EQ(root->node_extent<memorial::domain::thought_tag>(), 1U);
    EXPECT_EQ(branch->node_extent<memorial::domain::thought_tag>(), 2U);
    EXPECT_FALSE(root->contains(*local).value());
}

TEST(Snapshot, BranchDeltaCanConnectInheritedEndpointsWithoutCopyingThem) {
    memorial::delta_store<memorial::memorial_schema> root_delta;
    const row_context context;
    const auto experience = root_delta.nodes<memorial::domain::experience_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.9F);
    const auto perception = root_delta.nodes<memorial::domain::perception_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.8F);
    ASSERT_TRUE(experience);
    ASSERT_TRUE(perception);
    const auto root = snapshot_type::publish(memorial::generation_id{3U}, context.worldline,
                                             memorial::timestamp{15ns}, memorial::timestamp{35ns},
                                             std::move(root_delta));
    ASSERT_TRUE(root);

    auto branch_delta = root->make_branch_delta();
    const auto edge =
        branch_delta
            .append_edge<memorial::domain::experience_tag, memorial::domain::generates_relation,
                         memorial::domain::perception_tag>(*experience, *perception, 0.75);
    ASSERT_TRUE(edge);
    const auto branch = snapshot_type::publish_branch(
        *root, memorial::generation_id{4U}, memorial::worldline_id{2U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(branch_delta));
    ASSERT_TRUE(branch);

    const auto outgoing =
        branch->outgoing<memorial::domain::experience_tag, memorial::domain::generates_relation,
                         memorial::domain::perception_tag>(*experience);
    ASSERT_TRUE(outgoing);
    ASSERT_EQ(outgoing->size(), 1U);
    EXPECT_EQ(outgoing->front(), *edge);
}

TEST(Snapshot, RejectsBranchDeltaNotDerivedFromParent) {
    memorial::delta_store<memorial::memorial_schema> root_delta;
    const row_context context;
    ASSERT_TRUE(root_delta.nodes<memorial::domain::thought_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.7F, 0.8F));
    const auto root = snapshot_type::publish(memorial::generation_id{3U}, context.worldline,
                                             memorial::timestamp{15ns}, memorial::timestamp{35ns},
                                             std::move(root_delta));
    ASSERT_TRUE(root);

    memorial::delta_store<memorial::memorial_schema> unrelated;
    const auto branch = snapshot_type::publish_branch(
        *root, memorial::generation_id{4U}, memorial::worldline_id{2U}, memorial::timestamp{15ns},
        memorial::timestamp{35ns}, std::move(unrelated));
    ASSERT_FALSE(branch);
    EXPECT_EQ(branch.error().code(), memorial::graph_errc::conflict);
}

} // namespace
