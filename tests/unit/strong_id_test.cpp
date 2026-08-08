#include <memorial/id/strong_id.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_set>

namespace {

struct memory_tag;
using memory_id = memorial::node_id<memory_tag>;

TEST(StrongId, DefaultsToInvalid) {
    const memory_id id;

    EXPECT_FALSE(id.is_valid());
    EXPECT_FALSE(static_cast<bool>(id));
    EXPECT_EQ(id, memory_id::invalid());
}

TEST(StrongId, PreservesRawValue) {
    constexpr std::uint32_t raw = 42;
    const memory_id id{raw};

    EXPECT_TRUE(id.is_valid());
    EXPECT_EQ(id.value(), raw);
}

TEST(StrongId, SupportsHashContainers) {
    const std::unordered_set<memory_id> ids{memory_id{3}, memory_id{5}};

    EXPECT_TRUE(ids.contains(memory_id{3}));
    EXPECT_FALSE(ids.contains(memory_id{4}));
}

} // namespace
