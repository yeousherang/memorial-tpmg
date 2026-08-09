#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>

#include <gtest/gtest.h>

#include <chrono>

namespace {

using namespace std::chrono_literals;

TEST(TemporalInterval, AcceptsHalfOpenIntervals) {
    const memorial::timestamp start{10ns};
    const memorial::timestamp end{20ns};
    const auto interval = memorial::valid_interval::make(start, end);

    ASSERT_TRUE(interval);
    EXPECT_TRUE(interval->contains(start));
    EXPECT_TRUE(interval->contains(memorial::timestamp{19ns}));
    EXPECT_FALSE(interval->contains(end));
}

TEST(TemporalInterval, RejectsEmptyAndReversedIntervals) {
    const memorial::timestamp time{10ns};
    const auto empty = memorial::valid_interval::make(time, time);
    const auto reversed = memorial::transaction_interval::make(time, memorial::timestamp{9ns});

    ASSERT_FALSE(empty);
    ASSERT_FALSE(reversed);
    EXPECT_EQ(empty.error().code(), memorial::graph_errc::invalid_interval);
    EXPECT_EQ(reversed.error().code(), memorial::graph_errc::invalid_interval);
}

TEST(Provenance, OwnsItsSourceAndAcceptsObservedData) {
    std::string source = "participant journal";
    auto value = memorial::provenance::make(memorial::provenance_kind::self_reported, source);
    source.clear();

    ASSERT_TRUE(value);
    EXPECT_EQ(value->source(), "participant journal");
    EXPECT_FALSE(value->model_run());
}

TEST(Provenance, RequiresModelRunForInferredAndSimulatedData) {
    const auto missing =
        memorial::provenance::make(memorial::provenance_kind::inferred, "model registry");
    const auto valid = memorial::provenance::make(memorial::provenance_kind::simulated,
                                                  "model registry", memorial::model_run_id{7U});

    ASSERT_FALSE(missing);
    EXPECT_EQ(missing.error().code(), memorial::graph_errc::missing_provenance);
    ASSERT_TRUE(valid);
    ASSERT_TRUE(valid->model_run());
    EXPECT_EQ(valid->model_run()->value(), 7U);
}

TEST(Provenance, RejectsInvalidOptionalModelRun) {
    const auto value = memorial::provenance::make(memorial::provenance_kind::observed, "sensor",
                                                  memorial::model_run_id::invalid());

    ASSERT_FALSE(value);
    EXPECT_EQ(value.error().code(), memorial::graph_errc::invalid_id);
}

} // namespace
