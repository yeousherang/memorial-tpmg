#include <memorial/storage/node_store.hpp>

#include <gtest/gtest.h>

#include <string>

namespace {

struct person_tag {};
struct memory_layer {};

using person =
    memorial::node_spec<person_tag, memory_layer, memorial::property_spec<"name", std::string>,
                        memorial::property_spec<"confidence", float>>;

TEST(NodeStore, AppendsRowsAndReturnsTypedSequentialIds) {
    memorial::node_store<person> nodes;

    const auto ada = nodes.append("Ada", 0.9F);
    const auto grace = nodes.append("Grace", 0.8F);

    ASSERT_TRUE(ada);
    ASSERT_TRUE(grace);
    EXPECT_EQ(ada->value(), 0U);
    EXPECT_EQ(grace->value(), 1U);
    EXPECT_EQ(nodes.size(), 2U);
    EXPECT_EQ(nodes.column<"name">().values.size(), nodes.size());
    EXPECT_EQ(nodes.column<"confidence">().values.size(), nodes.size());
}

TEST(NodeStore, LooksUpAndMutatesAPropertyByTypedId) {
    memorial::node_store<person> nodes;
    const auto id = nodes.append("Ada", 0.9F);
    ASSERT_TRUE(id);

    auto confidence = nodes.property<"confidence">(*id);
    ASSERT_TRUE(confidence);
    confidence->get() = 0.95F;

    const auto& read_only_nodes = nodes;
    const auto name = read_only_nodes.property<"name">(*id);
    const auto updated_confidence = read_only_nodes.property<"confidence">(*id);
    ASSERT_TRUE(name);
    ASSERT_TRUE(updated_confidence);
    EXPECT_EQ(name->get(), "Ada");
    EXPECT_FLOAT_EQ(updated_confidence->get(), 0.95F);
}

TEST(NodeStore, DistinguishesInvalidAndUnknownIds) {
    memorial::node_store<person> nodes;
    ASSERT_TRUE(nodes.append("Ada", 0.9F));

    const auto invalid = nodes.property<"name">(person::id_type::invalid());
    const auto unknown = nodes.property<"name">(person::id_type{42U});

    ASSERT_FALSE(invalid);
    ASSERT_FALSE(unknown);
    EXPECT_EQ(invalid.error(), memorial::storage_error::invalid_id);
    EXPECT_EQ(unknown.error(), memorial::storage_error::id_not_found);
}

} // namespace
