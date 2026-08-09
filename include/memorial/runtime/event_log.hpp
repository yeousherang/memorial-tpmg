#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace memorial {

enum class event_kind : std::uint8_t {
    entity_created,
    state_appended,
    edge_created,
    worldline_forked,
    memory_reinterpreted,
    model_inference_recorded,
    snapshot_compacted,
    tombstone,
};

template <typename Payload>
concept EventPayload = std::movable<Payload> && std::is_object_v<Payload>;

template <EventPayload Payload> class graph_event {
  public:
    using payload_type = Payload;

    [[nodiscard]] event_sequence sequence() const noexcept { return sequence_; }
    [[nodiscard]] event_kind kind() const noexcept { return kind_; }
    [[nodiscard]] timestamp valid_time() const noexcept { return valid_time_; }
    [[nodiscard]] timestamp recorded_time() const noexcept { return recorded_time_; }
    [[nodiscard]] worldline_id worldline() const noexcept { return worldline_; }
    [[nodiscard]] const payload_type& payload() const noexcept { return payload_; }
    [[nodiscard]] std::optional<event_sequence> previous() const noexcept { return previous_; }
    [[nodiscard]] const provenance& source() const noexcept { return source_; }
    [[nodiscard]] std::uint64_t checksum() const noexcept { return checksum_; }

  private:
    template <EventPayload, typename> friend class event_log;

    graph_event(event_sequence sequence, event_kind kind, timestamp valid_time,
                timestamp recorded_time, worldline_id worldline, payload_type payload,
                std::optional<event_sequence> previous, provenance source, std::uint64_t checksum)
        : sequence_{sequence}, kind_{kind}, valid_time_{valid_time}, recorded_time_{recorded_time},
          worldline_{worldline}, payload_{std::move(payload)}, previous_{previous},
          source_{std::move(source)}, checksum_{checksum} {}

    event_sequence sequence_;
    event_kind kind_;
    timestamp valid_time_;
    timestamp recorded_time_;
    worldline_id worldline_;
    payload_type payload_;
    std::optional<event_sequence> previous_;
    provenance source_;
    std::uint64_t checksum_;
};

template <EventPayload Payload, typename PayloadHash = std::hash<Payload>> class event_log {
  public:
    using payload_type = Payload;
    using event_type = graph_event<payload_type>;
    using size_type = std::size_t;

    static_assert(std::regular_invocable<const PayloadHash&, const Payload&>,
                  "event payload hash must be callable with a const payload reference");
    static_assert(
        std::convertible_to<std::invoke_result_t<const PayloadHash&, const Payload&>, std::size_t>,
        "event payload hash result must be convertible to size_t");

    event_log() = default;
    explicit event_log(PayloadHash payload_hash) : payload_hash_{std::move(payload_hash)} {}

    [[nodiscard]] size_type size() const noexcept { return events_.size(); }
    [[nodiscard]] bool empty() const noexcept { return events_.empty(); }
    [[nodiscard]] std::span<const event_type> events() const noexcept { return events_; }

    [[nodiscard]] result<event_sequence> append(event_kind kind, timestamp valid_time,
                                                timestamp recorded_time, worldline_id worldline,
                                                payload_type payload, provenance source,
                                                std::optional<event_sequence> previous = {}) {
        if (!worldline.is_valid()) {
            return std::unexpected(
                graph_error{graph_errc::invalid_id, "event worldline ID must be valid"});
        }
        if (events_.size() >= static_cast<size_type>(event_sequence::invalid_value)) {
            return std::unexpected(graph_error{graph_errc::capacity_exceeded});
        }
        if (!events_.empty() && recorded_time < events_.back().recorded_time()) {
            return std::unexpected(graph_error{
                graph_errc::conflict, "event recorded time must be monotonically non-decreasing"});
        }
        if (previous && (!previous->is_valid() ||
                         static_cast<size_type>(previous->value()) >= events_.size())) {
            return std::unexpected(
                graph_error{graph_errc::id_not_found, "previous event reference does not exist"});
        }

        const auto sequence =
            event_sequence{static_cast<event_sequence::value_type>(events_.size())};
        const auto checksum =
            checksum_for(sequence, kind, valid_time, recorded_time, worldline, payload, previous);
        events_.push_back(event_type{sequence, kind, valid_time, recorded_time, worldline,
                                     std::move(payload), previous, std::move(source), checksum});
        return sequence;
    }

    [[nodiscard]] result<std::reference_wrapper<const event_type>>
    find(event_sequence sequence) const {
        if (!sequence.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        const auto index = static_cast<size_type>(sequence.value());
        if (index >= events_.size()) {
            return std::unexpected(graph_error{graph_errc::id_not_found});
        }
        return std::cref(events_[index]);
    }

  private:
    [[nodiscard]] static constexpr std::uint64_t mix(std::uint64_t seed,
                                                     std::uint64_t value) noexcept {
        return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
    }

    [[nodiscard]] std::uint64_t checksum_for(event_sequence sequence, event_kind kind,
                                             timestamp valid_time, timestamp recorded_time,
                                             worldline_id worldline, const payload_type& payload,
                                             std::optional<event_sequence> previous) const {
        auto checksum = static_cast<std::uint64_t>(payload_hash_(payload));
        checksum = mix(checksum, sequence.value());
        checksum = mix(checksum, static_cast<std::uint64_t>(kind));
        checksum = mix(checksum, static_cast<std::uint64_t>(valid_time.time_since_epoch().count()));
        checksum =
            mix(checksum, static_cast<std::uint64_t>(recorded_time.time_since_epoch().count()));
        checksum = mix(checksum, worldline.value());
        return mix(checksum, previous ? previous->value() : event_sequence::invalid_value);
    }

    std::vector<event_type> events_;
    [[no_unique_address]] PayloadHash payload_hash_{};
};

} // namespace memorial
