#include <memorial/query/dsl.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <gtest/gtest.h>

#include <chrono>

namespace {

using namespace std::chrono_literals;
using namespace memorial::query;

TEST(QueryDsl, OwnsRuntimeOperandsInTheExpressionTree) {
    const auto query = from<memorial::memorial_schema, memorial::domain::thought_tag>() |
                       active_at(memorial::timestamp{12ns}) |
                       in_worldline(memorial::worldline_id{3U}) |
                       where(prop<"confidence"> > 0.75F) | limit(8U);

    EXPECT_EQ(query.expression().operation.count, 8U);
    const auto& where_pipeline = query.expression().previous;
    EXPECT_FLOAT_EQ(where_pipeline.operation.predicate.value, 0.75F);
    EXPECT_EQ(where_pipeline.previous.operation.value, memorial::worldline_id{3U});
    EXPECT_EQ(where_pipeline.previous.previous.operation.value, memorial::timestamp{12ns});
}

TEST(QueryDsl, PreservesTopKDirectionAndCount) {
    const auto query = from<memorial::memorial_schema, memorial::domain::emotion_tag>() |
                       project<"valence">() | top_k<4>(by<"valence">.ascending());

    EXPECT_EQ(query.expression().operation.count, 4U);
    EXPECT_EQ(query.expression().operation.order.direction, sort_direction::ascending);
}

} // namespace
