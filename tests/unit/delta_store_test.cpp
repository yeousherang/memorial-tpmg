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

} // namespace
