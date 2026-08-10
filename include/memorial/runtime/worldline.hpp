#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/runtime/event_log.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace memorial {

enum class worldline_kind {
    actual,
    simulated,
};

class worldline_record {
  public:
    [[nodiscard]] worldline_id id() const noexcept { return id_; }
    [[nodiscard]] worldline_kind kind() const noexcept { return kind_; }
    [[nodiscard]] std::optional<worldline_id> parent() const noexcept { return parent_; }
    [[nodiscard]] std::optional<event_sequence> fork_event() const noexcept { return fork_event_; }
    [[nodiscard]] std::optional<timestamp> fork_valid_time() const noexcept {
        return fork_valid_time_;
    }
    [[nodiscard]] std::optional<intervention_id> intervention() const noexcept {
        return intervention_;
    }
    [[nodiscard]] generation_id generation() const noexcept { return generation_; }
    [[nodiscard]] const provenance& source() const noexcept { return source_; }

  private:
    friend class worldline_registry;

    worldline_record(worldline_id id, worldline_kind kind, std::optional<worldline_id> parent,
                     std::optional<event_sequence> fork_event,
                     std::optional<timestamp> fork_valid_time,
                     std::optional<intervention_id> intervention, generation_id generation,
                     provenance source)
        : id_{id}, kind_{kind}, parent_{parent}, fork_event_{fork_event},
          fork_valid_time_{fork_valid_time}, intervention_{intervention}, generation_{generation},
          source_{std::move(source)} {}

    worldline_id id_;
    worldline_kind kind_;
    std::optional<worldline_id> parent_;
    std::optional<event_sequence> fork_event_;
    std::optional<timestamp> fork_valid_time_;
    std::optional<intervention_id> intervention_;
    generation_id generation_;
    provenance source_;
};

class worldline_registry {
  public:
    [[nodiscard]] std::size_t size() const noexcept { return records_.size(); }
    [[nodiscard]] bool empty() const noexcept { return records_.empty(); }

    [[nodiscard]] result<worldline_id> create_root(generation_id generation, provenance source) {
        if (!records_.empty()) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "worldline registry already has a root"});
        }
        if (!generation.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        if (source.kind() == provenance_kind::simulated) {
            return std::unexpected(graph_error{
                graph_errc::conflict, "actual root worldline cannot use simulated provenance"});
        }

        const worldline_id id{0U};
        records_.push_back(worldline_record{
            id, worldline_kind::actual, {}, {}, {}, {}, generation, std::move(source)});
        return id;
    }

    template <EventPayload Payload, typename PayloadHash>
    [[nodiscard]] result<worldline_id>
    fork(worldline_id parent, const event_log<Payload, PayloadHash>& log, event_sequence fork_event,
         intervention_id intervention, generation_id generation, provenance simulation_source) {
        const auto parent_record = find(parent);
        if (!parent_record) {
            return std::unexpected(parent_record.error());
        }
        const auto event = log.find(fork_event);
        if (!event) {
            return std::unexpected(event.error());
        }
        if (event->get().kind() != event_kind::worldline_forked) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "fork event has the wrong event kind"});
        }
        if (event->get().worldline() != parent) {
            return std::unexpected(graph_error{graph_errc::worldline_mismatch,
                                               "fork event was not recorded on the parent"});
        }
        if (!intervention.is_valid() || !generation.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        if (simulation_source.kind() != provenance_kind::simulated) {
            return std::unexpected(graph_error{graph_errc::missing_provenance,
                                               "forked worldline requires simulated provenance"});
        }
        if (records_.size() >= static_cast<std::size_t>(worldline_id::invalid_value)) {
            return std::unexpected(graph_error{graph_errc::capacity_exceeded});
        }

        const auto id = worldline_id{static_cast<worldline_id::value_type>(records_.size())};
        records_.push_back(worldline_record{id, worldline_kind::simulated, parent, fork_event,
                                            event->get().valid_time(), intervention, generation,
                                            std::move(simulation_source)});
        return id;
    }

    [[nodiscard]] result<std::reference_wrapper<const worldline_record>>
    find(worldline_id id) const {
        if (!id.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        const auto index = static_cast<std::size_t>(id.value());
        if (index >= records_.size()) {
            return std::unexpected(graph_error{graph_errc::id_not_found});
        }
        return std::cref(records_[index]);
    }

    [[nodiscard]] result<std::reference_wrapper<const worldline_record>>
    parent_of(worldline_id id) const {
        const auto record = find(id);
        if (!record) {
            return std::unexpected(record.error());
        }
        if (!record->get().parent()) {
            return std::unexpected(
                graph_error{graph_errc::id_not_found, "root worldline has no parent"});
        }
        return find(*record->get().parent());
    }

    [[nodiscard]] result<std::vector<worldline_id>> lineage(worldline_id id) const {
        std::vector<worldline_id> result;
        auto current = find(id);
        if (!current) {
            return std::unexpected(current.error());
        }
        while (true) {
            result.push_back(current->get().id());
            if (!current->get().parent()) {
                break;
            }
            current = find(*current->get().parent());
            if (!current) {
                return std::unexpected(current.error());
            }
        }
        std::ranges::reverse(result);
        return result;
    }

  private:
    std::deque<worldline_record> records_;
};

} // namespace memorial
