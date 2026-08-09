#include <memorial/runtime/canonical_hash.hpp>
#include <memorial/runtime/memory_history.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>

namespace {

using namespace std::chrono_literals;
using memory_id = memorial::node_id<memorial::domain::memory_tag>;
using history_type = memorial::memory_history<memory_id, std::string>;

memorial::provenance self_reported_source() {
    return *memorial::provenance::make(memorial::provenance_kind::self_reported,
                                       "participant journal");
}

memorial::valid_interval remembered_period() {
    return *memorial::valid_interval::make(memorial::timestamp{100ns}, memorial::timestamp{200ns});
}

TEST(MemoryHistory, DistinguishesThenKnownFromCurrentlyKnownHistory) {
    history_type history;
    const memory_id memory{7U};
    const memorial::perspective_id perspective{1U};
    const memorial::worldline_id actual{1U};
    ASSERT_TRUE(history.append_original(memory, perspective, remembered_period(),
                                        memorial::timestamp{110ns}, actual, "I failed",
                                        self_reported_source()));
    ASSERT_TRUE(history.reinterpret(memory, perspective, remembered_period(),
                                    memorial::timestamp{150ns}, actual,
                                    "I learned from the attempt", self_reported_source()));

    const auto then_known = history.at(memory, perspective, memorial::timestamp{120ns},
                                       memorial::timestamp{120ns}, actual);
    const auto currently_known = history.at(memory, perspective, memorial::timestamp{120ns},
                                            memorial::timestamp{180ns}, actual);

    ASSERT_TRUE(then_known);
    ASSERT_TRUE(currently_known);
    EXPECT_EQ(then_known->get().payload().value, "I failed");
    EXPECT_EQ(currently_known->get().payload().value, "I learned from the attempt");
    EXPECT_EQ(then_known->get().payload().memory, currently_known->get().payload().memory);
}

TEST(MemoryHistory, ReinterpretationReferencesThePreviousVersion) {
    history_type history;
    const memory_id memory{7U};
    const memorial::perspective_id perspective{1U};
    const memorial::worldline_id actual{1U};
    const auto original =
        history.append_original(memory, perspective, remembered_period(),
                                memorial::timestamp{110ns}, actual, "old", self_reported_source());
    ASSERT_TRUE(original);
    const auto revision =
        history.reinterpret(memory, perspective, remembered_period(), memorial::timestamp{150ns},
                            actual, "new", self_reported_source());

    ASSERT_TRUE(revision);
    const auto event = history.log().find(*revision);
    ASSERT_TRUE(event);
    EXPECT_EQ(event->get().kind(), memorial::event_kind::memory_reinterpreted);
    EXPECT_EQ(event->get().previous(), *original);
}

TEST(MemoryHistory, KeepsPerspectivesIndependent) {
    history_type history;
    const memory_id memory{7U};
    const memorial::worldline_id actual{1U};
    ASSERT_TRUE(history.append_original(memory, memorial::perspective_id{1U}, remembered_period(),
                                        memorial::timestamp{110ns}, actual, "my account",
                                        self_reported_source()));
    ASSERT_TRUE(history.append_original(memory, memorial::perspective_id{2U}, remembered_period(),
                                        memorial::timestamp{111ns}, actual, "observer account",
                                        self_reported_source()));

    const auto mine = history.at(memory, memorial::perspective_id{1U}, memorial::timestamp{120ns},
                                 memorial::timestamp{180ns}, actual);
    const auto theirs = history.at(memory, memorial::perspective_id{2U}, memorial::timestamp{120ns},
                                   memorial::timestamp{180ns}, actual);

    ASSERT_TRUE(mine);
    ASSERT_TRUE(theirs);
    EXPECT_EQ(mine->get().payload().value, "my account");
    EXPECT_EQ(theirs->get().payload().value, "observer account");
}

TEST(MemoryHistory, RejectsReinterpretationWithoutAnEarlierVersion) {
    history_type history;

    const auto revision = history.reinterpret(
        memory_id{7U}, memorial::perspective_id{1U}, remembered_period(),
        memorial::timestamp{150ns}, memorial::worldline_id{1U}, "new", self_reported_source());

    ASSERT_FALSE(revision);
    EXPECT_EQ(revision.error().code(), memorial::graph_errc::id_not_found);
    EXPECT_TRUE(history.empty());
}

TEST(MemoryHistory, AppliesHalfOpenValidTimeBoundaries) {
    history_type history;
    ASSERT_TRUE(history.append_original(
        memory_id{7U}, memorial::perspective_id{1U}, remembered_period(),
        memorial::timestamp{110ns}, memorial::worldline_id{1U}, "account", self_reported_source()));

    const auto outside =
        history.at(memory_id{7U}, memorial::perspective_id{1U}, memorial::timestamp{200ns},
                   memorial::timestamp{180ns}, memorial::worldline_id{1U});

    ASSERT_FALSE(outside);
    EXPECT_EQ(outside.error().code(), memorial::graph_errc::id_not_found);
}

TEST(MemoryHistory, CanonicalLogHashIncludesReinterpretations) {
    history_type original_only;
    history_type reinterpreted;
    ASSERT_TRUE(original_only.append_original(
        memory_id{7U}, memorial::perspective_id{1U}, remembered_period(),
        memorial::timestamp{110ns}, memorial::worldline_id{1U}, "old", self_reported_source()));
    ASSERT_TRUE(reinterpreted.append_original(
        memory_id{7U}, memorial::perspective_id{1U}, remembered_period(),
        memorial::timestamp{110ns}, memorial::worldline_id{1U}, "old", self_reported_source()));
    ASSERT_TRUE(reinterpreted.reinterpret(
        memory_id{7U}, memorial::perspective_id{1U}, remembered_period(),
        memorial::timestamp{150ns}, memorial::worldline_id{1U}, "new", self_reported_source()));

    EXPECT_NE(memorial::canonical_hash_of(original_only.log()),
              memorial::canonical_hash_of(reinterpreted.log()));
}

} // namespace
