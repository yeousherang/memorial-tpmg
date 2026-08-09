#pragma once

#include <memorial/runtime/error.hpp>

#include <chrono>
#include <compare>

namespace memorial {

using timestamp = std::chrono::sys_time<std::chrono::nanoseconds>;

struct valid_time_domain {};
struct transaction_time_domain {};

template <typename Domain> class temporal_interval {
  public:
    [[nodiscard]] static result<temporal_interval> make(timestamp from, timestamp to) {
        if (from >= to) {
            return std::unexpected(
                graph_error{graph_errc::invalid_interval, "interval must satisfy from < to"});
        }
        return temporal_interval{from, to};
    }

    [[nodiscard]] static result<temporal_interval> open_ended(timestamp from) {
        return make(from, timestamp::max());
    }

    [[nodiscard]] timestamp from() const noexcept { return from_; }
    [[nodiscard]] timestamp to() const noexcept { return to_; }

    [[nodiscard]] bool contains(timestamp point) const noexcept {
        return from_ <= point && point < to_;
    }

    friend constexpr auto operator<=>(const temporal_interval&,
                                      const temporal_interval&) noexcept = default;

  private:
    constexpr temporal_interval(timestamp from, timestamp to) noexcept : from_{from}, to_{to} {}

    timestamp from_;
    timestamp to_;
};

using valid_interval = temporal_interval<valid_time_domain>;
using transaction_interval = temporal_interval<transaction_time_domain>;

} // namespace memorial
