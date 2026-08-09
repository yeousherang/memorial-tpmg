#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/runtime/event_log.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

namespace memorial {

template <typename State>
concept CheckpointState = std::copy_constructible<State> && std::movable<State>;

template <CheckpointState State> class checkpoint {
  public:
    using state_type = State;

    [[nodiscard]] static checkpoint initial(state_type state) {
        return checkpoint{std::move(state), {}, {}};
    }

    [[nodiscard]] static checkpoint anchored(state_type state, event_sequence sequence,
                                             std::uint64_t checksum) {
        return checkpoint{std::move(state), sequence, checksum};
    }

    [[nodiscard]] const state_type& state() const noexcept { return state_; }
    [[nodiscard]] std::optional<event_sequence> sequence() const noexcept { return sequence_; }
    [[nodiscard]] std::optional<std::uint64_t> checksum() const noexcept { return checksum_; }

  private:
    checkpoint(state_type state, std::optional<event_sequence> sequence,
               std::optional<std::uint64_t> checksum)
        : state_{std::move(state)}, sequence_{sequence}, checksum_{checksum} {}

    state_type state_;
    std::optional<event_sequence> sequence_;
    std::optional<std::uint64_t> checksum_;
};

template <EventPayload Payload, typename PayloadHash, CheckpointState State>
[[nodiscard]] result<checkpoint<State>> make_checkpoint(const event_log<Payload, PayloadHash>& log,
                                                        event_sequence sequence, State state) {
    const auto event = log.find(sequence);
    if (!event) {
        return std::unexpected(event.error());
    }
    return checkpoint<State>::anchored(std::move(state), sequence, event->get().checksum());
}

template <typename Reducer, typename State, typename Event>
concept ReplayReducer =
    std::invocable<Reducer&, State&, const Event&> &&
    std::same_as<std::invoke_result_t<Reducer&, State&, const Event&>, result<void>>;

template <EventPayload Payload, typename PayloadHash, CheckpointState State, typename Reducer>
    requires ReplayReducer<Reducer, State, graph_event<Payload>>
[[nodiscard]] result<State> replay(const event_log<Payload, PayloadHash>& log,
                                   const checkpoint<State>& origin, Reducer reducer,
                                   std::optional<event_sequence> through = {}) {
    std::size_t first_index{};
    if (origin.sequence()) {
        if (!origin.checksum()) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "anchored checkpoint is missing its checksum"});
        }
        const auto anchor = log.find(*origin.sequence());
        if (!anchor) {
            return std::unexpected(anchor.error());
        }
        if (anchor->get().checksum() != *origin.checksum()) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "checkpoint does not match the event log"});
        }
        first_index = static_cast<std::size_t>(origin.sequence()->value()) + 1U;
    }

    std::size_t last_index = log.size();
    if (through) {
        const auto last = log.find(*through);
        if (!last) {
            return std::unexpected(last.error());
        }
        last_index = static_cast<std::size_t>(through->value()) + 1U;
        if (last_index < first_index) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "replay end precedes the checkpoint sequence"});
        }
    }

    State state = origin.state();
    const auto events = log.events();
    for (auto index = first_index; index < last_index; ++index) {
        auto reduced = std::invoke(reducer, state, events[index]);
        if (!reduced) {
            return std::unexpected(reduced.error());
        }
    }
    return state;
}

} // namespace memorial
