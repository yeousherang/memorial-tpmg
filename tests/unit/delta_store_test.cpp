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
        *memorial::provenance::make(memorial::provenance_kind::self_reported, "journal");
};

TEST(DeltaStore, AppendsAndReadsACompleteTemporalRow) {
    memorial::delta_store<memorial::memorial_schema> delta;
    auto& thoughts = delta.nodes<memorial::domain::thought_tag>();
    const row_context context;

    const auto id = thoughts.append(context.worldline, context.valid, context.transaction,
                                    context.source, 0.7F, 0.8F);

    ASSERT_TRUE(id);
    EXPECT_EQ(thoughts.size(), 1U);
    ASSERT_TRUE(thoughts.property<"activation">(*id));
    EXPECT_FLOAT_EQ(thoughts.property<"activation">(*id)->get(), 0.7F);
    ASSERT_TRUE(thoughts.valid(*id));
    EXPECT_TRUE(thoughts.valid(*id)->get().contains(memorial::timestamp{15ns}));
    ASSERT_TRUE(thoughts.worldline(*id));
    EXPECT_EQ(*thoughts.worldline(*id), context.worldline);
    ASSERT_TRUE(thoughts.source(*id));
    EXPECT_EQ(thoughts.source(*id)->get().source(), "journal");
}

TEST(DeltaStore, KeepsNodeKindsInIndependentTypedStores) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const row_context context;

    ASSERT_TRUE(delta.nodes<memorial::domain::thought_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.7F, 0.8F));
    ASSERT_TRUE(delta.nodes<memorial::domain::emotion_tag>().append(
        context.worldline, context.valid, context.transaction, context.source, 0.5F, -0.2F));

    EXPECT_EQ(delta.nodes<memorial::domain::thought_tag>().size(), 1U);
    EXPECT_EQ(delta.nodes<memorial::domain::emotion_tag>().size(), 1U);
}

TEST(DeltaStore, RejectsInvalidWorldlineWithoutAppending) {
    memorial::delta_store<memorial::memorial_schema> delta;
    auto& thoughts = delta.nodes<memorial::domain::thought_tag>();
    const row_context context;

    const auto id = thoughts.append(memorial::worldline_id::invalid(), context.valid,
                                    context.transaction, context.source, 0.7F, 0.8F);

    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().code(), memorial::graph_errc::invalid_id);
    EXPECT_TRUE(thoughts.empty());
}

TEST(DeltaStore, ReportsUnknownTypedIds) {
    memorial::delta_store<memorial::memorial_schema> delta;
    const auto value = delta.nodes<memorial::domain::thought_tag>().valid(
        memorial::domain::thought_node::id_type{4U});

    ASSERT_FALSE(value);
    EXPECT_EQ(value.error().code(), memorial::graph_errc::id_not_found);
}

TEST(DeltaStore, BuildsTemporalAndWorldlineSelectionIndices) {
    memorial::delta_store<memorial::memorial_schema> delta;
    auto& thoughts = delta.nodes<memorial::domain::thought_tag>();
    const row_context context;
    const auto later =
        *memorial::valid_interval::make(memorial::timestamp{20ns}, memorial::timestamp{30ns});

    const auto visible = thoughts.append(context.worldline, context.valid, context.transaction,
                                         context.source, 0.7F, 0.8F);
    const auto other_worldline = thoughts.append(memorial::worldline_id{2U}, context.valid,
                                                 context.transaction, context.source, 0.6F, 0.7F);
    const auto not_yet_valid =
        thoughts.append(context.worldline, later, context.transaction, context.source, 0.5F, 0.6F);
    ASSERT_TRUE(visible);
    ASSERT_TRUE(other_worldline);
    ASSERT_TRUE(not_yet_valid);

    delta.build_indices();
    const auto candidates = thoughts.indexed_candidates(
        context.worldline, memorial::timestamp{15ns}, memorial::timestamp{35ns});

    ASSERT_TRUE(candidates);
    ASSERT_EQ(candidates->size(), 1U);
    EXPECT_EQ(candidates->front(), *visible);
}

TEST(DeltaStore, InvalidatesSelectionIndicesAfterAppend) {
    memorial::delta_store<memorial::memorial_schema> delta;
    auto& thoughts = delta.nodes<memorial::domain::thought_tag>();
    const row_context context;
    ASSERT_TRUE(thoughts.append(context.worldline, context.valid, context.transaction,
                                context.source, 0.7F, 0.8F));
    delta.build_indices();
    ASSERT_TRUE(thoughts.indices_ready());

    ASSERT_TRUE(thoughts.append(context.worldline, context.valid, context.transaction,
                                context.source, 0.6F, 0.7F));
    EXPECT_FALSE(thoughts.indices_ready());
    const auto stale = thoughts.indexed_candidates(context.worldline, memorial::timestamp{15ns},
                                                   memorial::timestamp{35ns});
    ASSERT_FALSE(stale);
    EXPECT_EQ(stale.error().code(), memorial::graph_errc::conflict);
}

TEST(DeltaStore, BuildsTypedPropertySelectionIndices) {
    memorial::delta_store<memorial::memorial_schema> delta;
    auto& thoughts = delta.nodes<memorial::domain::thought_tag>();
    const row_context context;
    const auto weak = thoughts.append(context.worldline, context.valid, context.transaction,
                                      context.source, 0.2F, 0.5F);
    const auto strong = thoughts.append(context.worldline, context.valid, context.transaction,
                                        context.source, 0.9F, 0.8F);
    ASSERT_TRUE(weak);
    ASSERT_TRUE(strong);
    delta.build_indices();

    const auto candidates = thoughts.indexed_property_candidates<"activation">(
        0.5F, [](float property, float threshold) { return property > threshold; });

    ASSERT_TRUE(candidates);
    ASSERT_EQ(candidates->size(), 1U);
    EXPECT_EQ(candidates->front(), *strong);
}

TEST(DeltaStore, EstimatesNumericPropertySelectivityFromHistogram) {
    memorial::delta_store<memorial::memorial_schema> delta;
    auto& thoughts = delta.nodes<memorial::domain::thought_tag>();
    const row_context context;
    for (std::size_t index = 0; index < 10U; ++index) {
        const auto activation = static_cast<float>(index) / 10.0F;
        ASSERT_TRUE(thoughts.append(context.worldline, context.valid, context.transaction,
                                    context.source, activation, 0.8F));
    }
    delta.build_indices();

    const auto estimate = thoughts.estimate_property_candidates<"activation">(
        0.75F, [](float property, float threshold) { return property > threshold; });

    ASSERT_TRUE(estimate);
    EXPECT_EQ(*estimate, 2U);
}

} // namespace
