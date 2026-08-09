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

} // namespace
