#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/time.hpp>
#include <memorial/schema/graph_schema.hpp>
#include <memorial/storage/delta_store.hpp>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace memorial {

template <GraphSchema Schema> class snapshot {
  public:
    using schema_type = Schema;

    [[nodiscard]] static result<snapshot> publish(generation_id generation, worldline_id worldline,
                                                  timestamp valid_at, timestamp known_at,
                                                  delta_store<schema_type>&& delta) {
        if (!generation.is_valid()) {
            return std::unexpected(
                graph_error{graph_errc::invalid_id, "snapshot generation ID must be valid"});
        }
        if (!worldline.is_valid()) {
            return std::unexpected(
                graph_error{graph_errc::invalid_id, "snapshot worldline ID must be valid"});
        }
        return snapshot{generation, worldline, valid_at, known_at,
                        std::make_shared<const delta_store<schema_type>>(std::move(delta))};
    }

    [[nodiscard]] generation_id generation() const noexcept { return generation_; }
    [[nodiscard]] worldline_id worldline() const noexcept { return worldline_; }
    [[nodiscard]] timestamp valid_at() const noexcept { return valid_at_; }
    [[nodiscard]] timestamp known_at() const noexcept { return known_at_; }

    template <typename Tag>
        requires schema_has_node_v<Tag, schema_type>
    [[nodiscard]] result<bool> contains(node_id<Tag> id) const {
        const auto visible = check_visibility<Tag>(id);
        if (!visible) {
            if (visible.error().code() == graph_errc::id_not_found ||
                visible.error().code() == graph_errc::worldline_mismatch) {
                return false;
            }
            return std::unexpected(visible.error());
        }
        return true;
    }

    template <typename Tag, meta::fixed_string Key>
        requires schema_has_node_v<Tag, schema_type> &&
                 node_has_property_v<schema_node_t<Tag, schema_type>, Key>
    [[nodiscard]] auto property(node_id<Tag> id) const -> result<
        std::reference_wrapper<const node_property_value_t<schema_node_t<Tag, schema_type>, Key>>> {
        const auto visible = check_visibility<Tag>(id);
        if (!visible) {
            return std::unexpected(visible.error());
        }
        return data_->template nodes<Tag>().template property<Key>(id);
    }

    template <typename Source, typename Relation, typename Target>
        requires schema_has_edge_v<Source, Relation, Target, schema_type>
    [[nodiscard]] auto outgoing(node_id<Source> source) const -> result<std::vector<
        typename adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>::id_type>> {
        const auto source_visible = check_visibility<Source>(source);
        if (!source_visible) {
            return std::unexpected(source_visible.error());
        }

        using edge_store_type =
            adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>;
        std::vector<typename edge_store_type::id_type> visible_edges;
        const auto& edges = data_->template edges<Source, Relation, Target>();
        for (const auto edge : edges.outgoing(source)) {
            const auto target = edges.target(edge);
            if (!target) {
                return std::unexpected(target.error());
            }
            const auto target_visible = contains<Target>(*target);
            if (!target_visible) {
                return std::unexpected(target_visible.error());
            }
            if (*target_visible) {
                visible_edges.push_back(edge);
            }
        }
        return visible_edges;
    }

    template <typename Source, typename Relation, typename Target>
        requires schema_has_edge_v<Source, Relation, Target, schema_type>
    [[nodiscard]] auto incoming(node_id<Target> target) const -> result<std::vector<
        typename adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>::id_type>> {
        const auto target_visible = check_visibility<Target>(target);
        if (!target_visible) {
            return std::unexpected(target_visible.error());
        }

        using edge_store_type =
            adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>;
        std::vector<typename edge_store_type::id_type> visible_edges;
        const auto& edges = data_->template edges<Source, Relation, Target>();
        for (const auto edge : edges.incoming(target)) {
            const auto source = edges.source(edge);
            if (!source) {
                return std::unexpected(source.error());
            }
            const auto source_visible = contains<Source>(*source);
            if (!source_visible) {
                return std::unexpected(source_visible.error());
            }
            if (*source_visible) {
                visible_edges.push_back(edge);
            }
        }
        return visible_edges;
    }

  private:
    snapshot(generation_id generation, worldline_id worldline, timestamp valid_at,
             timestamp known_at, std::shared_ptr<const delta_store<schema_type>> data) noexcept
        : generation_{generation}, worldline_{worldline}, valid_at_{valid_at}, known_at_{known_at},
          data_{std::move(data)} {}

    template <typename Tag> [[nodiscard]] result<void> check_visibility(node_id<Tag> id) const {
        const auto& nodes = data_->template nodes<Tag>();
        const auto row_worldline = nodes.worldline(id);
        if (!row_worldline) {
            return std::unexpected(row_worldline.error());
        }
        if (*row_worldline != worldline_) {
            return std::unexpected(
                graph_error{graph_errc::worldline_mismatch, "node belongs to another worldline"});
        }

        const auto valid = nodes.valid(id);
        const auto transaction = nodes.transaction(id);
        if (!valid) {
            return std::unexpected(valid.error());
        }
        if (!transaction) {
            return std::unexpected(transaction.error());
        }
        if (!valid->get().contains(valid_at_) || !transaction->get().contains(known_at_)) {
            return std::unexpected(
                graph_error{graph_errc::id_not_found, "node is not active in this snapshot"});
        }
        return {};
    }

    generation_id generation_;
    worldline_id worldline_;
    timestamp valid_at_;
    timestamp known_at_;
    std::shared_ptr<const delta_store<schema_type>> data_;
};

} // namespace memorial
