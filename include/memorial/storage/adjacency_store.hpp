#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/schema/edge.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace memorial {

template <PropertySpec Property> struct edge_property_column {
    using property = Property;
    using value_type = typename Property::value_type;

    std::vector<value_type> values;
};

template <EdgeSpec Edge> class adjacency_store;

template <typename Source, typename Relation, typename Target, PropertySpec... Properties>
class adjacency_store<edge_spec<Source, Relation, Target, Properties...>> {
  public:
    using edge_type = edge_spec<Source, Relation, Target, Properties...>;
    using id_type = strong_id<edge_type, std::uint32_t>;
    using source_id_type = typename edge_type::source_id_type;
    using target_id_type = typename edge_type::target_id_type;
    using size_type = std::size_t;

    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    template <typename... Values>
        requires(sizeof...(Values) == sizeof...(Properties)) &&
                (std::constructible_from<typename Properties::value_type, Values &&> && ...)
    [[nodiscard]] result<id_type> append(source_id_type source, target_id_type target,
                                         Values&&... values) {
        if (!source.is_valid() || !target.is_valid()) {
            return std::unexpected(
                graph_error{graph_errc::invalid_id, "edge endpoints must be valid"});
        }
        if (size_ >= static_cast<size_type>(id_type::invalid_value)) {
            return std::unexpected(graph_error{graph_errc::capacity_exceeded});
        }

        auto pending =
            std::tuple<source_id_type, target_id_type, typename Properties::value_type...>{
                source, target, std::forward<Values>(values)...};
        append_columns<0>(std::move(pending));

        const auto id = id_type{static_cast<typename id_type::value_type>(size_)};
        try {
            outgoing_[source].push_back(id);
            try {
                incoming_[target].push_back(id);
            } catch (...) {
                outgoing_[source].pop_back();
                throw;
            }
        } catch (...) {
            pop_columns();
            throw;
        }

        ++size_;
        return id;
    }

    [[nodiscard]] result<source_id_type> source(id_type id) const {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::get<0>(columns_)[*row];
    }

    [[nodiscard]] result<target_id_type> target(id_type id) const {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::get<1>(columns_)[*row];
    }

    template <meta::fixed_string Key>
        requires edge_has_property_v<edge_type, Key>
    [[nodiscard]] auto property(id_type id) const
        -> result<std::reference_wrapper<const edge_property_value_t<edge_type, Key>>> {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::cref(
            std::get<edge_property_column<edge_property_t<edge_type, Key>>>(columns_).values[*row]);
    }

    [[nodiscard]] std::span<const id_type> outgoing(source_id_type source) const noexcept {
        const auto found = outgoing_.find(source);
        return found == outgoing_.end() ? std::span<const id_type>{}
                                        : std::span<const id_type>{found->second};
    }

    [[nodiscard]] std::span<const id_type> incoming(target_id_type target) const noexcept {
        const auto found = incoming_.find(target);
        return found == incoming_.end() ? std::span<const id_type>{}
                                        : std::span<const id_type>{found->second};
    }

  private:
    using columns_type = std::tuple<std::vector<source_id_type>, std::vector<target_id_type>,
                                    edge_property_column<Properties>...>;

    [[nodiscard]] result<size_type> checked_row(id_type id) const {
        if (!id.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        const auto row = static_cast<size_type>(id.value());
        if (row >= size_) {
            return std::unexpected(graph_error{graph_errc::id_not_found});
        }
        return row;
    }

    template <size_type Index, typename Tuple> void append_columns(Tuple&& pending) {
        if constexpr (Index < std::tuple_size_v<columns_type>) {
            auto& column = std::get<Index>(columns_);
            if constexpr (Index < 2) {
                column.push_back(std::get<Index>(std::forward<Tuple>(pending)));
            } else {
                column.values.push_back(std::get<Index>(std::forward<Tuple>(pending)));
            }
            try {
                append_columns<Index + 1>(std::forward<Tuple>(pending));
            } catch (...) {
                if constexpr (Index < 2) {
                    column.pop_back();
                } else {
                    column.values.pop_back();
                }
                throw;
            }
        }
    }

    void pop_columns() noexcept {
        pop_columns(std::make_index_sequence<std::tuple_size_v<columns_type>>{});
    }

    template <size_type Index> void pop_column() noexcept {
        if constexpr (Index < 2) {
            std::get<Index>(columns_).pop_back();
        } else {
            std::get<Index>(columns_).values.pop_back();
        }
    }

    template <size_type... Indices> void pop_columns(std::index_sequence<Indices...>) noexcept {
        (pop_column<Indices>(), ...);
    }

    columns_type columns_;
    std::unordered_map<source_id_type, std::vector<id_type>> outgoing_;
    std::unordered_map<target_id_type, std::vector<id_type>> incoming_;
    size_type size_{};
};

} // namespace memorial
