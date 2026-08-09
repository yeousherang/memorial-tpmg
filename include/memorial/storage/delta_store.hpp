#pragma once

#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>
#include <memorial/schema/graph_schema.hpp>
#include <memorial/storage/adjacency_store.hpp>
#include <memorial/storage/node_store.hpp>

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

namespace memorial {

template <NodeSpec Node> class delta_node_store {
  public:
    using node_type = Node;
    using id_type = typename node_type::id_type;
    using size_type = std::size_t;

    [[nodiscard]] size_type size() const noexcept { return nodes_.size(); }
    [[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }
    [[nodiscard]] bool contains(id_type id) const noexcept { return nodes_.contains(id); }

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
        return static_cast<size_type>(id.value());
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

    node_store<node_type> nodes_;
    metadata_columns metadata_;
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

    template <typename Source, typename Relation, typename Target>
        requires schema_has_edge_v<Source, Relation, Target, schema_type>
    [[nodiscard]] auto edges() const noexcept
        -> const adjacency_store<schema_edge_t<Source, Relation, Target, schema_type>>& {
        using edge_type = schema_edge_t<Source, Relation, Target, schema_type>;
        return std::get<adjacency_store<edge_type>>(edge_stores_.values);
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
        if (!nodes<Source>().contains(source) || !nodes<Target>().contains(target)) {
            return std::unexpected(
                graph_error{graph_errc::id_not_found, "edge endpoint does not exist"});
        }
        using edge_type = schema_edge_t<Source, Relation, Target, schema_type>;
        return mutable_edges<edge_type>().append(source, target, std::forward<Values>(values)...);
    }

  private:
    template <EdgeSpec Edge> [[nodiscard]] auto mutable_edges() noexcept -> adjacency_store<Edge>& {
        return std::get<adjacency_store<Edge>>(edge_stores_.values);
    }

    detail::delta_node_stores<typename schema_type::nodes> node_stores_;
    detail::delta_edge_stores<typename schema_type::edges> edge_stores_;
};

} // namespace memorial
