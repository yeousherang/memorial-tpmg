#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

namespace memorial {

template <typename Value>
concept IdValue = std::unsigned_integral<Value> && !std::same_as<Value, bool>;

template <typename Tag, IdValue Value = std::uint32_t> class strong_id {
  public:
    using tag_type = Tag;
    using value_type = Value;

    static constexpr value_type invalid_value = std::numeric_limits<value_type>::max();

    constexpr strong_id() noexcept = default;
    explicit constexpr strong_id(value_type value) noexcept : value_{value} {}

    [[nodiscard]] static constexpr strong_id invalid() noexcept { return {}; }

    [[nodiscard]] constexpr value_type value() const noexcept { return value_; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return value_ != invalid_value; }
    [[nodiscard]] explicit constexpr operator bool() const noexcept { return is_valid(); }

    friend constexpr auto operator<=>(strong_id, strong_id) noexcept = default;

  private:
    value_type value_{invalid_value};
};

template <typename Type> struct is_strong_id : std::false_type {};

template <typename Tag, IdValue Value>
struct is_strong_id<strong_id<Tag, Value>> : std::true_type {};

template <typename Type>
inline constexpr bool is_strong_id_v = is_strong_id<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept StrongId = is_strong_id_v<Type>;

template <typename Tag> using node_id = strong_id<Tag, std::uint32_t>;

} // namespace memorial

namespace std {

template <typename Tag, memorial::IdValue Value> struct hash<memorial::strong_id<Tag, Value>> {
    [[nodiscard]] constexpr std::size_t operator()(memorial::strong_id<Tag, Value> id) const
        noexcept(noexcept(std::hash<Value>{}(id.value()))) {
        return std::hash<Value>{}(id.value());
    }
};

} // namespace std
