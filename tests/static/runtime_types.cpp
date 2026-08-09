#include <memorial/runtime/error.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>

#include <chrono>
#include <concepts>
#include <expected>
#include <type_traits>

static_assert(memorial::StrongId<memorial::worldline_id>);
static_assert(memorial::StrongId<memorial::generation_id>);
static_assert(memorial::StrongId<memorial::model_run_id>);
static_assert(!std::same_as<memorial::worldline_id, memorial::generation_id>);
static_assert(!std::same_as<memorial::valid_interval, memorial::transaction_interval>);
static_assert(std::same_as<memorial::result<int>, std::expected<int, memorial::graph_error>>);
static_assert(std::is_nothrow_copy_constructible_v<memorial::worldline_id>);

int main() {}
