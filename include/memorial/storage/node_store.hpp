#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/schema/node.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace memorial {

template <PropertySpec Property> struct property_column {
    using property = Property;
    using value_type = typename Property::value_type;

    std::vector<value_type> values;
};

template <NodeSpec Node> class node_store;

template <typename Tag, typename Layer, PropertySpec... Properties>
class node_store<node_spec<Tag, Layer, Properties...>> {
  public:
    using node_type = node_spec<Tag, Layer, Properties...>;
    using id_type = typename node_type::id_type;
    using size_type = std::size_t;

    node_store() = default;

    explicit node_store(size_type id_base) noexcept : id_base_{id_base} {}

    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] size_type id_base() const noexcept { return id_base_; }
    [[nodiscard]] size_type extent() const noexcept { return id_base_ + size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] bool contains(id_type id) const noexcept {
        if (!id.is_valid()) {
            return false;
        }
        const auto value = static_cast<size_type>(id.value());
        return value >= id_base_ && value < extent();
    }

    template <typename... Values>
        requires(sizeof...(Values) == sizeof...(Properties)) &&
                (std::constructible_from<typename Properties::value_type, Values &&> && ...)
    [[nodiscard]] result<id_type> append(Values&&... values) {
        if (extent() >= static_cast<size_type>(id_type::invalid_value)) {
            return std::unexpected(graph_error{graph_errc::capacity_exceeded});
        }

        auto pending =
            std::tuple<typename Properties::value_type...>{std::forward<Values>(values)...};
        append_columns<0>(std::move(pending));

        const auto id = id_type{static_cast<typename id_type::value_type>(extent())};
        ++size_;
        return id;
    }

    template <meta::fixed_string Key>
        requires node_has_property_v<node_type, Key>
    [[nodiscard]] auto property(id_type id)
        -> result<std::reference_wrapper<node_property_value_t<node_type, Key>>> {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::ref(column<Key>().values[*row]);
    }

    template <meta::fixed_string Key>
        requires node_has_property_v<node_type, Key>
    [[nodiscard]] auto property(id_type id) const
        -> result<std::reference_wrapper<const node_property_value_t<node_type, Key>>> {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::cref(column<Key>().values[*row]);
    }

    template <meta::fixed_string Key>
        requires node_has_property_v<node_type, Key>
    [[nodiscard]] auto column() noexcept -> property_column<node_property_t<node_type, Key>>& {
        return std::get<property_column<node_property_t<node_type, Key>>>(columns_);
    }

    template <meta::fixed_string Key>
        requires node_has_property_v<node_type, Key>
    [[nodiscard]] auto column() const noexcept
        -> const property_column<node_property_t<node_type, Key>>& {
        return std::get<property_column<node_property_t<node_type, Key>>>(columns_);
    }

  private:
    [[nodiscard]] result<size_type> checked_row(id_type id) const {
        if (!id.is_valid()) {
            return std::unexpected(graph_error{graph_errc::invalid_id});
        }
        const auto value = static_cast<size_type>(id.value());
        if (value < id_base_ || value >= extent()) {
            return std::unexpected(graph_error{graph_errc::id_not_found});
        }
        return value - id_base_;
    }

    template <size_type Index, typename Tuple> void append_columns(Tuple&& pending) {
        if constexpr (Index < sizeof...(Properties)) {
            auto& values = std::get<Index>(columns_).values;
            values.push_back(std::get<Index>(std::forward<Tuple>(pending)));
            try {
                append_columns<Index + 1>(std::forward<Tuple>(pending));
            } catch (...) {
                values.pop_back();
                throw;
            }
        }
    }

    std::tuple<property_column<Properties>...> columns_;
    size_type id_base_{};
    size_type size_{};
};

} // namespace memorial
