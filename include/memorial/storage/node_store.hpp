#pragma once

#include <memorial/schema/node.hpp>

#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace memorial {

enum class storage_error {
    invalid_id,
    id_not_found,
    capacity_exceeded,
};

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

    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] bool contains(id_type id) const noexcept {
        return id.is_valid() && static_cast<size_type>(id.value()) < size_;
    }

    template <typename... Values>
        requires(sizeof...(Values) == sizeof...(Properties)) &&
                (std::constructible_from<typename Properties::value_type, Values &&> && ...)
    [[nodiscard]] std::expected<id_type, storage_error> append(Values&&... values) {
        if (size_ >= static_cast<size_type>(id_type::invalid_value)) {
            return std::unexpected(storage_error::capacity_exceeded);
        }

        auto pending =
            std::tuple<typename Properties::value_type...>{std::forward<Values>(values)...};
        append_columns<0>(std::move(pending));

        const auto id = id_type{static_cast<typename id_type::value_type>(size_)};
        ++size_;
        return id;
    }

    template <meta::fixed_string Key>
        requires node_has_property_v<node_type, Key>
    [[nodiscard]] auto property(id_type id)
        -> std::expected<std::reference_wrapper<node_property_value_t<node_type, Key>>,
                         storage_error> {
        const auto row = checked_row(id);
        if (!row) {
            return std::unexpected(row.error());
        }
        return std::ref(column<Key>().values[*row]);
    }

    template <meta::fixed_string Key>
        requires node_has_property_v<node_type, Key>
    [[nodiscard]] auto property(id_type id) const
        -> std::expected<std::reference_wrapper<const node_property_value_t<node_type, Key>>,
                         storage_error> {
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
    [[nodiscard]] std::expected<size_type, storage_error> checked_row(id_type id) const noexcept {
        if (!id.is_valid()) {
            return std::unexpected(storage_error::invalid_id);
        }
        const auto row = static_cast<size_type>(id.value());
        if (row >= size_) {
            return std::unexpected(storage_error::id_not_found);
        }
        return row;
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
    size_type size_{};
};

} // namespace memorial
