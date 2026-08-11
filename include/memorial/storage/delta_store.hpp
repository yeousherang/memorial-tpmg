#pragma once

#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>
#include <memorial/schema/graph_schema.hpp>
#include <memorial/storage/adjacency_store.hpp>
#include <memorial/storage/node_store.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace memorial {

namespace detail {

inline constexpr std::size_t property_histogram_bucket_count = 16U;

struct property_histogram {
    std::array<std::size_t, property_histogram_bucket_count> buckets{};
    std::size_t sampled_count{};
    double minimum{};
    double maximum{};
    bool available{};
};

template <PropertySpec Property, StrongId Id> struct property_selection_index {
    std::vector<Id> rows;
    property_histogram histogram;
};

template <meta::TypeList Properties, StrongId Id> struct property_selection_indices;

template <PropertySpec... Properties, StrongId Id>
struct property_selection_indices<meta::type_list<Properties...>, Id> {
    std::tuple<property_selection_index<Properties, Id>...> values;
};

} // namespace detail

template <NodeSpec Node> class delta_node_store {
  public:
    using node_type = Node;
    using id_type = typename node_type::id_type;
    using size_type = std::size_t;

    delta_node_store() = default;
    explicit delta_node_store(size_type id_base) noexcept : nodes_{id_base} {}

    [[nodiscard]] size_type size() const noexcept { return nodes_.size(); }
    [[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }
    [[nodiscard]] bool contains(id_type id) const noexcept { return nodes_.contains(id); }
    [[nodiscard]] size_type id_base() const noexcept { return nodes_.id_base(); }
    [[nodiscard]] size_type extent() const noexcept { return nodes_.extent(); }
    [[nodiscard]] bool indices_ready() const noexcept { return indices_ready_; }

    void build_indices() {
        indices_ready_ = false;
        worldline_index_.clear();
        valid_start_index_.clear();
        transaction_start_index_.clear();
        valid_start_index_.reserve(size());
        transaction_start_index_.reserve(size());
        for (size_type row = 0; row < size(); ++row) {
            const auto id = id_type{static_cast<typename id_type::value_type>(id_base() + row)};
            worldline_index_[std::get<0>(metadata_)[row]].push_back(id);
            valid_start_index_.emplace_back(std::get<1>(metadata_)[row].from(), id);
            transaction_start_index_.emplace_back(std::get<2>(metadata_)[row].from(), id);
        }
        const auto by_time = [](const auto& left, const auto& right) {
            return left.first < right.first;
        };
        std::ranges::sort(valid_start_index_, by_time);
        std::ranges::sort(transaction_start_index_, by_time);
        build_property_indices(typename node_type::properties{});
        indices_ready_ = true;
    }

    template <meta::fixed_string Key, typename Value, typename Predicate>
        requires node_has_property_v<node_type, Key> &&
                 std::predicate<Predicate, const node_property_value_t<node_type, Key>&,
                                const Value&>
    [[nodiscard]] result<std::vector<id_type>>
    indexed_property_candidates(const Value& value, Predicate predicate) const {
        if (!indices_ready_) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "delta selection indices are not built"});
        }
        const auto& rows = property_index<Key>().rows;
        const auto& values = nodes_.template column<Key>().values;
        std::vector<id_type> result;
        for (const auto id : rows) {
            const auto row = static_cast<size_type>(id.value()) - id_base();
            if (std::invoke(predicate, values[row], value)) {
                result.push_back(id);
            }
        }
        std::ranges::sort(result,
                          [](id_type left, id_type right) { return left.value() < right.value(); });
        return result;
    }

    template <meta::fixed_string Key, typename Value, typename Predicate>
        requires node_has_property_v<node_type, Key> &&
                 std::predicate<Predicate, const node_property_value_t<node_type, Key>&,
                                const Value&>
    [[nodiscard]] result<size_type> estimate_property_candidates(const Value& value,
                                                                 Predicate predicate) const {
        if (!indices_ready_) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "delta selection indices are not built"});
        }
        using property_type = node_property_value_t<node_type, Key>;
        if constexpr (!std::is_arithmetic_v<property_type> || std::same_as<property_type, bool>) {
            return size();
        } else {
            const auto& histogram = property_index<Key>().histogram;
            if (!histogram.available) {
                return size();
            }
            if (histogram.minimum == histogram.maximum) {
                return std::invoke(predicate, static_cast<property_type>(histogram.minimum), value)
                           ? histogram.sampled_count
                           : 0U;
            }
            size_type estimate{};
            const auto width = (histogram.maximum - histogram.minimum) /
                               static_cast<double>(detail::property_histogram_bucket_count);
            for (size_type bucket = 0; bucket < detail::property_histogram_bucket_count; ++bucket) {
                const auto midpoint =
                    histogram.minimum + (static_cast<double>(bucket) + 0.5) * width;
                if (std::invoke(predicate, static_cast<property_type>(midpoint), value)) {
                    estimate += histogram.buckets[bucket];
                }
            }
            return estimate + (size() - histogram.sampled_count);
        }
    }

    [[nodiscard]] result<std::vector<id_type>>
    indexed_candidates(worldline_id worldline, timestamp valid_at, timestamp known_at) const {
        if (!indices_ready_) {
            return std::unexpected(
                graph_error{graph_errc::conflict, "delta selection indices are not built"});
        }
        const auto worldline_rows = worldline_index_.find(worldline);
        if (worldline_rows == worldline_index_.end()) {
            return std::vector<id_type>{};
        }
        const auto valid_end = std::upper_bound(
            valid_start_index_.begin(), valid_start_index_.end(), valid_at,
            [](timestamp point, const auto& entry) { return point < entry.first; });
        const auto transaction_end = std::upper_bound(
            transaction_start_index_.begin(), transaction_start_index_.end(), known_at,
            [](timestamp point, const auto& entry) { return point < entry.first; });
        const auto valid_count =
            static_cast<size_type>(std::distance(valid_start_index_.begin(), valid_end));
        const auto transaction_count = static_cast<size_type>(
            std::distance(transaction_start_index_.begin(), transaction_end));

        std::vector<id_type> result;
        result.reserve(std::min({worldline_rows->second.size(), valid_count, transaction_count}));
        const auto append_if_visible = [&](id_type id) {
            const auto row = static_cast<size_type>(id.value()) - id_base();
            if (std::get<0>(metadata_)[row] == worldline &&
                std::get<1>(metadata_)[row].contains(valid_at) &&
                std::get<2>(metadata_)[row].contains(known_at)) {
                result.push_back(id);
            }
        };
        if (worldline_rows->second.size() <= valid_count &&
            worldline_rows->second.size() <= transaction_count) {
            for (const auto id : worldline_rows->second) {
                append_if_visible(id);
            }
        } else if (valid_count <= transaction_count) {
            for (auto iterator = valid_start_index_.begin(); iterator != valid_end; ++iterator) {
                append_if_visible(iterator->second);
            }
        } else {
            for (auto iterator = transaction_start_index_.begin(); iterator != transaction_end;
                 ++iterator) {
                append_if_visible(iterator->second);
            }
        }
        std::ranges::sort(result,
                          [](id_type left, id_type right) { return left.value() < right.value(); });
        return result;
    }

    template <typename... Values>
        requires requires(node_store<node_type>& store, Values&&... values) {
            store.append(std::forward<Values>(values)...);
        }
    [[nodiscard]] result<id_type> append(worldline_id worldline, valid_interval valid,
                                         transaction_interval transaction, provenance source,
                                         Values&&... values) {
        if (!worldline.is_valid()) {
            return std::unexpected(
                graph_error{graph_errc::invalid_id, "delta row worldline ID must be valid"});
        }
        indices_ready_ = false;

        auto pending =
            std::tuple{worldline, std::move(valid), std::move(transaction), std::move(source)};
        append_metadata<0>(std::move(pending));
        try {
            auto id = nodes_.append(std::forward<Values>(values)...);
            if (!id) {
                pop_metadata();
            }
            return id;
        } catch (...) {
            pop_metadata();
            throw;
        }
    }

    template <meta::fixed_string Key>
        requires node_has_property_v<node_type, Key>
    [[nodiscard]] auto property(id_type id) const
        -> result<std::reference_wrapper<const node_property_value_t<node_type, Key>>> {
        return nodes_.template property<Key>(id);
    }

    [[nodiscard]] result<std::reference_wrapper<const valid_interval>> valid(id_type id) const {
        return metadata<1>(id);
    }

    [[nodiscard]] result<std::reference_wrapper<const transaction_interval>>
    transaction(id_type id) const {
        return metadata<2>(id);
    }

    [[nodiscard]] result<worldline_id> worldline(id_type id) const {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::get<0>(metadata_)[*row];
    }

    [[nodiscard]] result<std::reference_wrapper<const provenance>> source(id_type id) const {
        return metadata<3>(id);
    }

  private:
    using metadata_columns = std::tuple<std::vector<worldline_id>, std::vector<valid_interval>,
                                        std::vector<transaction_interval>, std::vector<provenance>>;

    [[nodiscard]] result<size_type> checked_row(id_type id) const {
        if (!id.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        if (!nodes_.contains(id)) {
            return std::unexpected(graph_error{graph_errc::id_not_found});
        }
        return static_cast<size_type>(id.value()) - nodes_.id_base();
    }

    template <size_type Index>
    [[nodiscard]] auto metadata(id_type id) const -> result<std::reference_wrapper<
        const typename std::tuple_element_t<Index, metadata_columns>::value_type>> {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::cref(std::get<Index>(metadata_)[*row]);
    }

    template <size_type Index, typename Tuple> void append_metadata(Tuple&& pending) {
        if constexpr (Index < std::tuple_size_v<metadata_columns>) {
            auto& values = std::get<Index>(metadata_);
            values.push_back(std::get<Index>(std::forward<Tuple>(pending)));
            try {
                append_metadata<Index + 1>(std::forward<Tuple>(pending));
            } catch (...) {
                values.pop_back();
                throw;
            }
        }
    }

    void pop_metadata() noexcept {
        std::apply([](auto&... columns) { (columns.pop_back(), ...); }, metadata_);
    }

    template <PropertySpec... Properties>
    void build_property_indices(meta::type_list<Properties...>) {
        (build_property_index<Properties>(), ...);
    }

    template <PropertySpec Property> void build_property_index() {
        auto& index =
            std::get<detail::property_selection_index<Property, id_type>>(property_indices_.values);
        auto& rows = index.rows;
        rows.clear();
        rows.reserve(size());
        for (size_type row = 0; row < size(); ++row) {
            rows.emplace_back(id_type{static_cast<typename id_type::value_type>(id_base() + row)});
        }
        build_property_histogram<Property>(index.histogram);
    }

    template <PropertySpec Property>
    void build_property_histogram(detail::property_histogram& histogram) {
        histogram = {};
        using value_type = typename Property::value_type;
        if constexpr (std::is_arithmetic_v<value_type> && !std::same_as<value_type, bool>) {
            const auto& values = nodes_.template column<Property::key>().values;
            bool initialized{};
            for (const auto value : values) {
                const auto sample = static_cast<double>(value);
                if (!std::isfinite(sample)) {
                    continue;
                }
                if (!initialized) {
                    histogram.minimum = sample;
                    histogram.maximum = sample;
                    initialized = true;
                } else {
                    histogram.minimum = std::min(histogram.minimum, sample);
                    histogram.maximum = std::max(histogram.maximum, sample);
                }
                ++histogram.sampled_count;
            }
            histogram.available = initialized;
            if (!initialized) {
                return;
            }
            const auto range = histogram.maximum - histogram.minimum;
            for (const auto value : values) {
                const auto sample = static_cast<double>(value);
                if (!std::isfinite(sample)) {
                    continue;
                }
                const auto scaled = range == 0.0 ? 0.0 : (sample - histogram.minimum) / range;
                const auto bucket = std::min(
                    static_cast<size_type>(scaled * detail::property_histogram_bucket_count),
                    detail::property_histogram_bucket_count - 1U);
                ++histogram.buckets[bucket];
            }
        }
    }

    template <meta::fixed_string Key> [[nodiscard]] const auto& property_index() const noexcept {
        using property = node_property_t<node_type, Key>;
        return std::get<detail::property_selection_index<property, id_type>>(
            property_indices_.values);
    }

    node_store<node_type> nodes_;
    metadata_columns metadata_;
    std::unordered_map<worldline_id, std::vector<id_type>> worldline_index_;
    std::vector<std::pair<timestamp, id_type>> valid_start_index_;
    std::vector<std::pair<timestamp, id_type>> transaction_start_index_;
    detail::property_selection_indices<typename node_type::properties, id_type> property_indices_;
    bool indices_ready_{};
};

namespace detail {

template <meta::TypeList Nodes> struct delta_node_stores;

template <NodeSpec... Nodes> struct delta_node_stores<meta::type_list<Nodes...>> {
    std::tuple<delta_node_store<Nodes>...> values;
};

template <meta::TypeList Edges> struct delta_edge_stores;

template <EdgeSpec... Edges> struct delta_edge_stores<meta::type_list<Edges...>> {
    std::tuple<adjacency_store<Edges>...> values;
};

} // namespace detail

template <GraphSchema Schema> class delta_store {
  public:
    using schema_type = Schema;

    void build_indices() {
        build_node_indices(typename schema_type::nodes{});
        build_edge_statistics(typename schema_type::edges{});
    }

    template <typename Parent> [[nodiscard]] static delta_store branch_from(const Parent& parent) {
        delta_store result;
        result.initialize_node_bases(parent, typename schema_type::nodes{});
        result.initialize_edge_bases(parent, typename schema_type::edges{});
        return result;
    }

    template <typename Tag>
        requires schema_has_node_v<Tag, schema_type>
    [[nodiscard]] auto nodes() noexcept -> delta_node_store<schema_node_t<Tag, schema_type>>& {
        return std::get<delta_node_store<schema_node_t<Tag, schema_type>>>(node_stores_.values);
    }

    template <typename Tag>
        requires schema_has_node_v<Tag, schema_type>
    [[nodiscard]] auto nodes() const noexcept
        -> const delta_node_store<schema_node_t<Tag, schema_type>>& {
        return std::get<delta_node_store<schema_node_t<Tag, schema_type>>>(node_stores_.values);
    }

    template <typename Tag>
        requires schema_has_node_v<Tag, schema_type>
    [[nodiscard]] std::size_t node_extent() const noexcept {
        return nodes<Tag>().extent();
    }

    template <typename Source, typename Relation, typename Target>
        requires schema_has_edge_v<Source, Relation, Target, schema_type>
    [[nodiscard]] auto edges() const noexcept
        -> const adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>& {
        using edge_type = schema_edge_t<Source, Relation, Target, schema_type>;
        return std::get<adjacency_store<edge_type>>(edge_stores_.values);
    }

    template <typename Source, typename Relation, typename Target>
        requires schema_has_edge_v<Source, Relation, Target, schema_type>
    [[nodiscard]] std::size_t edge_extent() const noexcept {
        return edges<Source, Relation, Target>().extent();
    }

    template <typename Source, typename Relation, typename Target, typename... Values>
        requires schema_has_edge_v<Source, Relation, Target, schema_type> &&
                 requires(
                     adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>& store,
                     node_id<Source> source, node_id<Target> target, Values&&... values) {
                     store.append(source, target, std::forward<Values>(values)...);
                 }
    [[nodiscard]] auto
    append_edge(node_id<Source> source, node_id<Target> target, Values&&... values) -> result<
        typename adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>::id_type> {
        if (!endpoint_exists<Source>(source) || !endpoint_exists<Target>(target)) {
            return std::unexpected(
                graph_error{graph_errc::id_not_found, "edge endpoint does not exist"});
        }
        if (nodes<Source>().contains(source) && nodes<Target>().contains(target)) {
            const auto source_worldline = nodes<Source>().worldline(source);
            const auto target_worldline = nodes<Target>().worldline(target);
            if (!source_worldline) {
                return std::unexpected(source_worldline.error());
            }
            if (!target_worldline) {
                return std::unexpected(target_worldline.error());
            }
            if (*source_worldline != *target_worldline) {
                return std::unexpected(
                    graph_error{graph_errc::worldline_mismatch,
                                "edge endpoints belong to different worldlines"});
            }
        }
        using edge_type = schema_edge_t<Source, Relation, Target, schema_type>;
        return mutable_edges<edge_type>().append(source, target, std::forward<Values>(values)...);
    }

  private:
    template <NodeSpec... Nodes> void build_node_indices(meta::type_list<Nodes...>) {
        (std::get<delta_node_store<Nodes>>(node_stores_.values).build_indices(), ...);
    }
    template <EdgeSpec... Edges> void build_edge_statistics(meta::type_list<Edges...>) {
        (std::get<adjacency_store<Edges>>(edge_stores_.values).build_statistics(), ...);
    }
    template <typename Tag> [[nodiscard]] bool endpoint_exists(node_id<Tag> id) const noexcept {
        return id.is_valid() && (static_cast<std::size_t>(id.value()) < nodes<Tag>().id_base() ||
                                 nodes<Tag>().contains(id));
    }

    template <typename Parent, NodeSpec... Nodes>
    void initialize_node_bases(const Parent& parent, meta::type_list<Nodes...>) {
        node_stores_.values = std::tuple<delta_node_store<Nodes>...>{
            delta_node_store<Nodes>{parent.template node_extent<typename Nodes::tag>()}...};
    }

    template <typename Parent, EdgeSpec... Edges>
    void initialize_edge_bases(const Parent& parent, meta::type_list<Edges...>) {
        edge_stores_.values = std::tuple<adjacency_store<Edges>...>{adjacency_store<Edges>{
            parent.template edge_extent<typename Edges::source, typename Edges::relation,
                                        typename Edges::target>()}...};
    }

    template <EdgeSpec Edge> [[nodiscard]] auto mutable_edges() noexcept -> adjacency_store<Edge>& {
        return std::get<adjacency_store<Edge>>(edge_stores_.values);
    }

    detail::delta_node_stores<typename schema_type::nodes> node_stores_;
    detail::delta_edge_stores<typename schema_type::edges> edge_stores_;
};

} // namespace memorial
