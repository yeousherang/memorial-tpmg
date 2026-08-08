#pragma once

#include <memorial/meta/type_list.hpp>

#include <concepts>
#include <type_traits>

namespace memorial::meta {

template <typename Key, typename Value> struct type_map_entry {
    using key_type = Key;
    using mapped_type = Value;
};

template <typename Type> struct is_type_map_entry : std::false_type {};

template <typename Key, typename Value>
struct is_type_map_entry<type_map_entry<Key, Value>> : std::true_type {};

template <typename Type> inline constexpr bool is_type_map_entry_v = is_type_map_entry<Type>::value;

template <typename Type>
concept TypeMapEntry = is_type_map_entry_v<Type>;

template <TypeMapEntry Entry> struct type_map_entry_key {
    using type = typename Entry::key_type;
};

template <TypeMapEntry... Entries> struct type_map {
    using entries = type_list<Entries...>;
    using keys = type_list_transform_t<type_map_entry_key, entries>;

    static_assert(type_list_is_unique_v<keys>, "type_map keys must be unique");

    static constexpr std::size_t size = sizeof...(Entries);
};

template <typename Type> struct is_type_map : std::false_type {};

template <TypeMapEntry... Entries> struct is_type_map<type_map<Entries...>> : std::true_type {};

template <typename Type>
inline constexpr bool is_type_map_v = is_type_map<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept TypeMap = is_type_map_v<Type>;

template <typename Key> struct type_map_key_matches {
    template <TypeMapEntry Entry> struct predicate : std::is_same<Key, typename Entry::key_type> {};
};

template <typename Key, TypeMap Map>
using type_map_find_entry_t =
    type_list_find_if_t<type_map_key_matches<Key>::template predicate, typename Map::entries>;

template <typename Key, TypeMap Map>
inline constexpr bool type_map_contains_v =
    !std::same_as<type_map_find_entry_t<Key, Map>, type_list_not_found>;

template <typename Key, TypeMap Map>
    requires type_map_contains_v<Key, Map>
struct type_map_at {
    using type = typename type_map_find_entry_t<Key, Map>::mapped_type;
};

template <typename Key, TypeMap Map> using type_map_at_t = typename type_map_at<Key, Map>::type;

} // namespace memorial::meta
