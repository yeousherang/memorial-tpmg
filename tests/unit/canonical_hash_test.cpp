#include <memorial/runtime/canonical_hash.hpp>
#include <memorial/runtime/replay.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <limits>
#include <string>

namespace {

using namespace std::chrono_literals;

struct counter_state {
    int value{};
};

void canonical_hash_append(memorial::canonical_hasher& hasher,
                           const counter_state& state) noexcept {
    hasher.append_tag("test.counter_state.v1");
    memorial::canonical_hash_append(hasher, state.value);
}

struct add_payload {
    int amount{};
};

void canonical_hash_append(memorial::canonical_hasher& hasher,
                           const add_payload& payload) noexcept {
    hasher.append_tag("test.add_payload.v1");
    memorial::canonical_hash_append(hasher, payload.amount);
}

struct payload_hash {
    std::size_t operator()(const add_payload& payload) const noexcept {
        return std::hash<int>{}(payload.amount);
    }
};

using log_type = memorial::event_log<add_payload, payload_hash>;

memorial::provenance observed_source() {
    return *memorial::provenance::make(memorial::provenance_kind::observed, "hash fixture");
}

void append(log_type& log, int amount, int recorded_nanoseconds) {
    ASSERT_TRUE(log.append(memorial::event_kind::state_appended, memorial::timestamp{10ns},
                           memorial::timestamp{std::chrono::nanoseconds{recorded_nanoseconds}},
                           memorial::worldline_id{1U}, add_payload{amount}, observed_source()));
}

memorial::result<void> reduce(counter_state& state, const log_type::event_type& event) {
    state.value += event.payload().amount;
    return {};
}

TEST(CanonicalHash, EqualReplayResultsProduceEqualDigests) {
    log_type log;
    append(log, 1, 20);
    append(log, 2, 21);
    const auto full =
        memorial::replay(log, memorial::checkpoint<counter_state>::initial({}), reduce);
    ASSERT_TRUE(full);
    const auto checkpoint =
        memorial::make_checkpoint(log, memorial::event_sequence{0U}, counter_state{1});
    ASSERT_TRUE(checkpoint);
    const auto resumed = memorial::replay(log, *checkpoint, reduce);
    ASSERT_TRUE(resumed);

    EXPECT_EQ(memorial::canonical_hash_of(*full), memorial::canonical_hash_of(*resumed));
}

TEST(CanonicalHash, EventOrderAndPayloadAffectLogDigest) {
    log_type first;
    log_type second;
    append(first, 1, 20);
    append(first, 2, 21);
    append(second, 2, 20);
    append(second, 1, 21);

    EXPECT_NE(memorial::canonical_hash_of(first), memorial::canonical_hash_of(second));
}

TEST(CanonicalHash, EqualLogsProduceEqualHexDigests) {
    log_type first;
    log_type second;
    append(first, 4, 20);
    append(second, 4, 20);

    const auto first_hash = memorial::canonical_hash_of(first);
    const auto second_hash = memorial::canonical_hash_of(second);
    EXPECT_EQ(first_hash, second_hash);
    EXPECT_EQ(first_hash.hex().size(), 16U);
}

TEST(CanonicalHash, NormalizesSignedZeroAndNan) {
    EXPECT_EQ(memorial::canonical_hash_of(0.0), memorial::canonical_hash_of(-0.0));
    EXPECT_EQ(memorial::canonical_hash_of(std::numeric_limits<double>::quiet_NaN()),
              memorial::canonical_hash_of(-std::numeric_limits<double>::quiet_NaN()));
}

TEST(CanonicalHash, TypeAndLengthMarkersPreventSimpleConcatenationCollisions) {
    EXPECT_NE(memorial::canonical_hash_of(std::string{"ab"}),
              memorial::canonical_hash_of(std::string{"a"}));
    EXPECT_NE(memorial::canonical_hash_of(std::uint8_t{1U}),
              memorial::canonical_hash_of(std::uint64_t{1U}));
}

} // namespace
