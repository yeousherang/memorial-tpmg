#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/runtime/event_log.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace memorial {

template <StrongId MemoryId, EventPayload Value> struct memory_revision {
    MemoryId memory;
    perspective_id perspective;
    valid_interval valid;
    Value value;
};

template <StrongId MemoryId, EventPayload Value, typename ValueHash = std::hash<Value>>
struct memory_revision_hash {
    std::size_t operator()(const memory_revision<MemoryId, Value>& revision) const
        noexcept(noexcept(std::declval<const ValueHash&>()(revision.value))) {
        auto seed = std::hash<MemoryId>{}(revision.memory);
        seed ^= std::hash<perspective_id>{}(revision.perspective) + 0x9e3779b9U + (seed << 6U) +
                (seed >> 2U);
        seed ^= static_cast<std::size_t>(revision.valid.from().time_since_epoch().count()) +
                0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        seed ^= static_cast<std::size_t>(revision.valid.to().time_since_epoch().count()) +
                0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        seed ^= static_cast<std::size_t>(ValueHash{}(revision.value)) + 0x9e3779b9U + (seed << 6U) +
                (seed >> 2U);
        return seed;
    }
};

template <StrongId MemoryId, EventPayload Value, typename ValueHash = std::hash<Value>>
    requires std::default_initializable<ValueHash> &&
             std::regular_invocable<const ValueHash&, const Value&>
class memory_history {
  public:
    using memory_id_type = MemoryId;
    using value_type = Value;
    using revision_type = memory_revision<memory_id_type, value_type>;
    using log_type =
        event_log<revision_type, memory_revision_hash<memory_id_type, value_type, ValueHash>>;
    using event_type = typename log_type::event_type;

    [[nodiscard]] std::size_t size() const noexcept { return log_.size(); }
    [[nodiscard]] bool empty() const noexcept { return log_.empty(); }

    [[nodiscard]] result<event_sequence>
    append_original(memory_id_type memory, perspective_id perspective, valid_interval valid,
                    timestamp recorded_time, worldline_id worldline, value_type value,
                    provenance source) {
        if (has_revision(memory, perspective, worldline)) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "memory already has a recorded interpretation"});
        }
        return append(event_kind::entity_created, memory, perspective, std::move(valid),
                      recorded_time, worldline, std::move(value), std::move(source), {});
    }

    [[nodiscard]] result<event_sequence> reinterpret(memory_id_type memory,
                                                     perspective_id perspective,
                                                     valid_interval valid, timestamp recorded_time,
                                                     worldline_id worldline, value_type value,
                                                     provenance source) {
        const auto previous = latest_sequence(memory, perspective, worldline, recorded_time);
        if (!previous) {
            return std::unexpected(graph_error{graph_errc::id_not_found,
                                               "memory has no earlier interpretation to revise"});
        }
        return append(event_kind::memory_reinterpreted, memory, perspective, std::move(valid),
                      recorded_time, worldline, std::move(value), std::move(source), previous);
    }

    [[nodiscard]] result<std::reference_wrapper<const event_type>>
    at(memory_id_type memory, perspective_id perspective, timestamp valid_at, timestamp known_at,
       worldline_id worldline) const {
        if (!memory.is_valid() || !perspective.is_valid() || !worldline.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }

        const event_type* selected{};
        for (const auto& event : log_.events()) {
            const auto& revision = event.payload();
            if (revision.memory == memory && revision.perspective == perspective &&
                event.worldline() == worldline && event.recorded_time() <= known_at &&
                revision.valid.contains(valid_at) &&
                (selected == nullptr || selected->recorded_time() < event.recorded_time() ||
                 (selected->recorded_time() == event.recorded_time() &&
                  selected->sequence() < event.sequence()))) {
                selected = &event;
            }
        }
        if (selected == nullptr) {
            return std::unexpected(graph_error{graph_errc::id_not_found,
                                               "memory is not visible at the requested times"});
        }
        return std::cref(*selected);
    }

    [[nodiscard]] const log_type& log() const noexcept { return log_; }

  private:
    [[nodiscard]] bool has_revision(memory_id_type memory, perspective_id perspective,
                                    worldline_id worldline) const noexcept {
        for (const auto& event : log_.events()) {
            if (event.payload().memory == memory && event.payload().perspective == perspective &&
                event.worldline() == worldline) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::optional<event_sequence>
    latest_sequence(memory_id_type memory, perspective_id perspective, worldline_id worldline,
                    timestamp recorded_time) const noexcept {
        std::optional<event_sequence> selected;
        for (const auto& event : log_.events()) {
            if (event.payload().memory == memory && event.payload().perspective == perspective &&
                event.worldline() == worldline && event.recorded_time() <= recorded_time) {
                selected = event.sequence();
            }
        }
        return selected;
    }

    [[nodiscard]] result<event_sequence> append(event_kind kind, memory_id_type memory,
                                                perspective_id perspective, valid_interval valid,
                                                timestamp recorded_time, worldline_id worldline,
                                                value_type value, provenance source,
                                                std::optional<event_sequence> previous) {
        if (!memory.is_valid() || !perspective.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        const auto valid_time = valid.from();
        return log_.append(kind, valid_time, recorded_time, worldline,
                           revision_type{memory, perspective, std::move(valid), std::move(value)},
                           std::move(source), previous);
    }

    log_type log_;
};

} // namespace memorial
