#pragma once

#include <memorial/id/strong_id.hpp>
#include <memorial/meta/type_list.hpp>
#include <memorial/schema/property.hpp>

#include <cstddef>
#include <type_traits>

namespace memorial {

template <typename Tag, typename Layer, PropertySpec... Properties> struct node_spec {
    using tag = Tag;
    using layer = Layer;
    using properties = meta::type_list<Properties...>;
    using id_type = node_id<Tag>;

    static_assert(property_keys_are_unique_v<properties>, "node property keys must be unique");

    static constexpr std::size_t property_count = sizeof...(Properties);
};

template <typename Type> struct is_node_spec : std::false_type {};

template <typename Tag, typename Layer, PropertySpec... Properties>
struct is_node_spec<node_spec<Tag, Layer, Properties...>> : std::true_type {};

template <typename Type>
inline constexpr bool is_node_spec_v = is_node_spec<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept NodeSpec = is_node_spec_v<Type>;

template <NodeSpec Node, meta::fixed_string Key>
inline constexpr bool node_has_property_v = has_property_v<Key, typename Node::properties>;

template <NodeSpec Node, meta::fixed_string Key>
    requires node_has_property_v<Node, Key>
using node_property_t = find_property_t<Key, typename Node::properties>;

template <NodeSpec Node, meta::fixed_string Key>
    requires node_has_property_v<Node, Key>
using node_property_value_t = property_value_t<Key, typename Node::properties>;

} // namespace memorial
