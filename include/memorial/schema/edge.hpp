#pragma once

#include <memorial/id/strong_id.hpp>
#include <memorial/meta/type_list.hpp>
#include <memorial/schema/property.hpp>

#include <cstddef>
#include <type_traits>

namespace memorial {

template <typename Source, typename Relation, typename Target, PropertySpec... Properties>
struct edge_spec {
    using source = Source;
    using relation = Relation;
    using target = Target;
    using properties = meta::type_list<Properties...>;
    using source_id_type = node_id<Source>;
    using target_id_type = node_id<Target>;

    static_assert(property_keys_are_unique_v<properties>, "edge property keys must be unique");

    static constexpr std::size_t property_count = sizeof...(Properties);
};

template <typename Type> struct is_edge_spec : std::false_type {};

template <typename Source, typename Relation, typename Target, PropertySpec... Properties>
struct is_edge_spec<edge_spec<Source, Relation, Target, Properties...>> : std::true_type {};

template <typename Type>
inline constexpr bool is_edge_spec_v = is_edge_spec<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept EdgeSpec = is_edge_spec_v<Type>;

template <EdgeSpec Edge, meta::fixed_string Key>
inline constexpr bool edge_has_property_v = has_property_v<Key, typename Edge::properties>;

template <EdgeSpec Edge, meta::fixed_string Key>
    requires edge_has_property_v<Edge, Key>
using edge_property_t = find_property_t<Key, typename Edge::properties>;

template <EdgeSpec Edge, meta::fixed_string Key>
    requires edge_has_property_v<Edge, Key>
using edge_property_value_t = property_value_t<Key, typename Edge::properties>;

} // namespace memorial
