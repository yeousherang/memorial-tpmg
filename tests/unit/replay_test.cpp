#include <memorial/runtime/replay.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <optional>

namespace {

using namespace std::chrono_literals;

struct counter_state {
    int value{};
};

struct add_payload {
    int amount{};
};

struct add_payload_hash {
    std::size_t operator()(const add_payload& payload) const noexcept {
        return std::hash<int>{}(payload.amount);
    }
};

using log_type = memorial::event_log<add_payload, add_payload_hash>;

memorial::provenance observed_source() {
    return *memorial::provenance::make(memorial::provenance_kind::observed, "counter fixture");
}

memorial::result<void> add(counter_state& state, const log_type::event_type& event) {
    state.value += event.payload().amount;
    return {};
}

memorial::event_sequence append(log_type& log, int amount, int recorded_nanoseconds) {
    return *log.append(memorial::event_kind::state_appended, memorial::timestamp{10ns},
                       memorial::timestamp{std::chrono::nanoseconds{recorded_nanoseconds}},
                       memorial::worldline_id{1U}, add_payload{amount}, observed_source());
}

TEST(Replay, FullReplayAndCheckpointReplayProduceTheSameState) {
    log_type log;
    append(log, 1, 20);
    const auto second = append(log, 2, 21);
    append(log, 3, 22);

    const auto full = memorial::replay(log, memorial::checkpoint<counter_state>::initial({}), add);
    const auto saved = memorial::make_checkpoint(log, second, counter_state{3});
    ASSERT_TRUE(saved);
    const auto resumed = memorial::replay(log, *saved, add);

    ASSERT_TRUE(full);
    ASSERT_TRUE(resumed);
    EXPECT_EQ(full->value, 6);
    EXPECT_EQ(resumed->value, full->value);
    EXPECT_EQ(saved->state().value, 3);
}

TEST(Replay, StopsAtAnInclusiveSequence) {
    log_type log;
    append(log, 1, 20);
    const auto second = append(log, 2, 21);
    append(log, 8, 22);

    const auto replayed =
        memorial::replay(log, memorial::checkpoint<counter_state>::initial({}), add, second);

    ASSERT_TRUE(replayed);
    EXPECT_EQ(replayed->value, 3);
}

TEST(Replay, RejectsCheckpointFromADifferentLogPrefix) {
    log_type first;
    log_type second;
    const auto sequence = append(first, 2, 20);
    append(second, 9, 20);
    const auto saved = memorial::make_checkpoint(first, sequence, counter_state{2});
    ASSERT_TRUE(saved);

    const auto replayed = memorial::replay(second, *saved, add);

    ASSERT_FALSE(replayed);
    EXPECT_EQ(replayed.error().code(), memorial::graph_errc::conflict);
}

TEST(Replay, PropagatesReducerErrorsWithoutChangingCheckpoint) {
    log_type log;
    append(log, 1, 20);
    append(log, -1, 21);
    const auto reducer = [](counter_state& state,
                            const log_type::event_type& event) -> memorial::result<void> {
        if (event.payload().amount < 0) {
            return std::unexpected(
                memorial::graph_error{memorial::graph_errc::conflict, "negative amount"});
        }
        state.value += event.payload().amount;
        return {};
    };
    const auto initial = memorial::checkpoint<counter_state>::initial({});

    const auto replayed = memorial::replay(log, initial, reducer);

    ASSERT_FALSE(replayed);
    EXPECT_EQ(replayed.error().code(), memorial::graph_errc::conflict);
    EXPECT_EQ(initial.state().value, 0);
}

TEST(Replay, RejectsEndBeforeCheckpoint) {
    log_type log;
    const auto first = append(log, 1, 20);
    const auto second = append(log, 2, 21);
    const auto saved = memorial::make_checkpoint(log, second, counter_state{3});
    ASSERT_TRUE(saved);

    const auto replayed = memorial::replay(log, *saved, add, first);

    ASSERT_FALSE(replayed);
    EXPECT_EQ(replayed.error().code(), memorial::graph_errc::conflict);
}

} // namespace
