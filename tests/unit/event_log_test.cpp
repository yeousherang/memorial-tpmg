#include <memorial/runtime/event_log.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>

namespace {

using namespace std::chrono_literals;

memorial::provenance observed_source() {
    return *memorial::provenance::make(memorial::provenance_kind::observed, "episode journal");
}

TEST(EventLog, AssignsContiguousSequenceNumbersAndOwnsPayloads) {
    memorial::event_log<std::string> log;
    std::string payload = "experience:0";

    const auto first = log.append(memorial::event_kind::entity_created, memorial::timestamp{10ns},
                                  memorial::timestamp{20ns}, memorial::worldline_id{1U}, payload,
                                  observed_source());
    ASSERT_TRUE(first);
    payload.clear();
    const auto second = log.append(memorial::event_kind::state_appended, memorial::timestamp{11ns},
                                   memorial::timestamp{21ns}, memorial::worldline_id{1U},
                                   "thought:0", observed_source(), *first);

    ASSERT_TRUE(second);
    EXPECT_EQ(first->value(), 0U);
    EXPECT_EQ(second->value(), 1U);
    ASSERT_EQ(log.events().size(), 2U);
    EXPECT_EQ(log.events().front().payload(), "experience:0");
    EXPECT_EQ(log.events().back().previous(), *first);
    EXPECT_NE(log.events().front().checksum(), 0U);
}

TEST(EventLog, FindsImmutableEventsBySequence) {
    memorial::event_log<std::string> log;
    const auto sequence = log.append(memorial::event_kind::edge_created, memorial::timestamp{10ns},
                                     memorial::timestamp{20ns}, memorial::worldline_id{1U},
                                     "edge:0", observed_source());
    ASSERT_TRUE(sequence);

    const auto event = log.find(*sequence);

    ASSERT_TRUE(event);
    EXPECT_EQ(event->get().kind(), memorial::event_kind::edge_created);
    EXPECT_EQ(event->get().worldline(), memorial::worldline_id{1U});
}

TEST(EventLog, RejectsInvalidWorldlineAndUnknownPreviousReference) {
    memorial::event_log<std::string> log;
    const auto invalid_worldline = log.append(
        memorial::event_kind::entity_created, memorial::timestamp{10ns}, memorial::timestamp{20ns},
        memorial::worldline_id::invalid(), "node", observed_source());
    const auto missing_previous = log.append(
        memorial::event_kind::state_appended, memorial::timestamp{10ns}, memorial::timestamp{20ns},
        memorial::worldline_id{1U}, "state", observed_source(), memorial::event_sequence{4U});

    ASSERT_FALSE(invalid_worldline);
    ASSERT_FALSE(missing_previous);
    EXPECT_EQ(invalid_worldline.error().code(), memorial::graph_errc::invalid_id);
    EXPECT_EQ(missing_previous.error().code(), memorial::graph_errc::id_not_found);
    EXPECT_TRUE(log.empty());
}

TEST(EventLog, RejectsRecordedTimeRegressionWithoutMutation) {
    memorial::event_log<std::string> log;
    ASSERT_TRUE(log.append(memorial::event_kind::entity_created, memorial::timestamp{10ns},
                           memorial::timestamp{30ns}, memorial::worldline_id{1U}, "node",
                           observed_source()));

    const auto regressed = log.append(memorial::event_kind::state_appended,
                                      memorial::timestamp{11ns}, memorial::timestamp{29ns},
                                      memorial::worldline_id{1U}, "state", observed_source());

    ASSERT_FALSE(regressed);
    EXPECT_EQ(regressed.error().code(), memorial::graph_errc::conflict);
    EXPECT_EQ(log.size(), 1U);
}

TEST(EventLog, ProducesEqualChecksumsForEqualEventEnvelopes) {
    memorial::event_log<std::string> first;
    memorial::event_log<std::string> second;

    ASSERT_TRUE(first.append(memorial::event_kind::entity_created, memorial::timestamp{10ns},
                             memorial::timestamp{20ns}, memorial::worldline_id{1U}, "node",
                             observed_source()));
    ASSERT_TRUE(second.append(memorial::event_kind::entity_created, memorial::timestamp{10ns},
                              memorial::timestamp{20ns}, memorial::worldline_id{1U}, "node",
                              observed_source()));

    EXPECT_EQ(first.events().front().checksum(), second.events().front().checksum());
}

} // namespace
