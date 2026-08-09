#pragma once

#include <memorial/meta/type_list.hpp>
#include <memorial/schema/edge.hpp>
#include <memorial/schema/node.hpp>
#include <memorial/schema/relation_rules.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace memorial {

template <typename List>
concept NodeSpecList = meta::TypeList<List> && meta::type_list_all_of_v<is_node_spec, List>;

template <typename List>
concept EdgeSpecList = meta::TypeList<List> && meta::type_list_all_of_v<is_edge_spec, List>;

template <NodeSpec Node> struct node_tag {
    using type = typename Node::tag;
};

template <typename Source, typename Relation, typename Target> struct edge_signature {};

template <EdgeSpec Edge> struct edge_signature_of {
    using type =
        edge_signature<typename Edge::source, typename Edge::relation, typename Edge::target>;
};

template <typename Tag> struct node_tag_matches {
    template <NodeSpec Node> struct predicate : std::is_same<Tag, typename Node::tag> {};
};

template <typename Source, typename Relation, typename Target> struct edge_signature_matches {
    template <EdgeSpec Edge>
    struct predicate : std::bool_constant<std::same_as<Source, typename Edge::source> &&
                                          std::same_as<Relation, typename Edge::relation> &&
                                          std::same_as<Target, typename Edge::target>> {};
};

template <NodeSpecList Nodes, EdgeSpecList Edges,
          RelationRulePolicy RelationRules = unrestricted_relation_rules>
struct graph_schema {
    using nodes = Nodes;
    using edges = Edges;
    using relation_rules = RelationRules;
    using node_tags = meta::type_list_transform_t<node_tag, nodes>;
    using edge_signatures = meta::type_list_transform_t<edge_signature_of, edges>;

    static_assert(meta::type_list_is_unique_v<node_tags>, "graph_schema node tags must be unique");
    static_assert(meta::type_list_is_unique_v<edge_signatures>,
                  "graph_schema edge signatures must be unique");

    template <EdgeSpec Edge>
    struct has_known_endpoints
        : std::bool_constant<meta::type_list_contains_v<typename Edge::source, node_tags> &&
                             meta::type_list_contains_v<typename Edge::target, node_tags>> {};

    static_assert(meta::type_list_all_of_v<has_known_endpoints, edges>,
                  "graph_schema edge endpoints must reference declared node tags");

    template <EdgeSpec Edge> struct has_allowed_layer_relation {
      private:
        using source_node =
            meta::type_list_find_if_t<node_tag_matches<typename Edge::source>::template predicate,
                                      nodes>;
        using target_node =
            meta::type_list_find_if_t<node_tag_matches<typename Edge::target>::template predicate,
                                      nodes>;

        [[nodiscard]] static consteval bool evaluate() {
            if constexpr (std::same_as<source_node, meta::type_list_not_found> ||
                          std::same_as<target_node, meta::type_list_not_found>) {
                return true;
            } else {
                return relation_rules::template allows<typename Edge::relation,
                                                       typename source_node::layer,
                                                       typename target_node::layer>;
            }
        }

      public:
        static constexpr bool value = evaluate();
    };

    static_assert(meta::type_list_all_of_v<has_allowed_layer_relation, edges>,
                  "graph_schema edge violates its relation layer rules");

    static constexpr std::size_t node_count = meta::type_list_size_v<nodes>;
    static constexpr std::size_t edge_count = meta::type_list_size_v<edges>;

    template <typename Tag>
    static constexpr bool contains_node =
        meta::type_list_any_of_v<node_tag_matches<Tag>::template predicate, nodes>;

    template <typename Source, typename Relation, typename Target>
    static constexpr bool contains_edge = meta::type_list_any_of_v<
        edge_signature_matches<Source, Relation, Target>::template predicate, edges>;
};

template <typename Type> struct is_graph_schema : std::false_type {};

template <NodeSpecList Nodes, EdgeSpecList Edges, RelationRulePolicy RelationRules>
struct is_graph_schema<graph_schema<Nodes, Edges, RelationRules>> : std::true_type {};

template <typename Type>
inline constexpr bool is_graph_schema_v = is_graph_schema<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept GraphSchema = is_graph_schema_v<Type>;

template <typename Tag, GraphSchema Schema>
using find_schema_node_t = meta::type_list_find_if_t<node_tag_matches<Tag>::template predicate,
                                                     typename std::remove_cvref_t<Schema>::nodes>;

template <typename Tag, GraphSchema Schema>
inline constexpr bool schema_has_node_v =
    !std::same_as<find_schema_node_t<Tag, Schema>, meta::type_list_not_found>;

template <typename Tag, GraphSchema Schema>
    requires schema_has_node_v<Tag, Schema>
using schema_node_t = find_schema_node_t<Tag, Schema>;

template <typename Source, typename Relation, typename Target, GraphSchema Schema>
using find_schema_edge_t =
    meta::type_list_find_if_t<edge_signature_matches<Source, Relation, Target>::template predicate,
                              typename std::remove_cvref_t<Schema>::edges>;

template <typename Source, typename Relation, typename Target, GraphSchema Schema>
inline constexpr bool schema_has_edge_v =
    !std::same_as<find_schema_edge_t<Source, Relation, Target, Schema>, meta::type_list_not_found>;

template <typename Source, typename Relation, typename Target, GraphSchema Schema>
    requires schema_has_edge_v<Source, Relation, Target, Schema>
using schema_edge_t = find_schema_edge_t<Source, Relation, Target, Schema>;

template <typename Schema, typename Source, typename Relation, typename Target>
concept ValidEdge = GraphSchema<Schema> && schema_has_edge_v<Source, Relation, Target, Schema>;

} // namespace memorial
