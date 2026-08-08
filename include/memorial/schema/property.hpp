#pragma once

#include <memorial/meta/fixed_string.hpp>
#include <memorial/meta/type_list.hpp>

#include <concepts>
#include <type_traits>

namespace memorial {

template <typename Type>
concept PropertyValue =
    std::is_object_v<Type> && !std::is_array_v<Type> && std::same_as<Type, std::remove_cv_t<Type>>;

template <meta::fixed_string Key, PropertyValue Value> struct property_spec {
    static_assert(!Key.empty(), "property key must not be empty");

    static constexpr auto key = Key;
    using value_type = Value;
};

template <typename Type> struct is_property_spec : std::false_type {};

template <meta::fixed_string Key, PropertyValue Value>
struct is_property_spec<property_spec<Key, Value>> : std::true_type {};

template <typename Type>
inline constexpr bool is_property_spec_v = is_property_spec<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept PropertySpec = is_property_spec_v<Type>;

template <PropertySpec Property> struct property_key {
    using type = std::integral_constant<decltype(Property::key), Property::key>;
};

template <meta::TypeList Properties>
using property_keys_t = meta::type_list_transform_t<property_key, Properties>;

template <meta::TypeList Properties>
inline constexpr bool property_keys_are_unique_v =
    meta::type_list_is_unique_v<property_keys_t<Properties>>;

template <meta::fixed_string Key> struct property_key_matches {
    template <PropertySpec Property> struct predicate : std::bool_constant<Property::key == Key> {};
};

template <meta::fixed_string Key, meta::TypeList Properties>
using find_property_t =
    meta::type_list_find_if_t<property_key_matches<Key>::template predicate, Properties>;

template <meta::fixed_string Key, meta::TypeList Properties>
inline constexpr bool has_property_v =
    !std::same_as<find_property_t<Key, Properties>, meta::type_list_not_found>;

template <meta::fixed_string Key, meta::TypeList Properties>
    requires has_property_v<Key, Properties>
using property_value_t = typename find_property_t<Key, Properties>::value_type;

} // namespace memorial
