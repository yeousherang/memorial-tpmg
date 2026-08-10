#pragma once

#include <memorial/meta/fixed_string.hpp>
#include <memorial/meta/type_list.hpp>
#include <memorial/query/ast.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/time.hpp>
#include <memorial/schema/graph_schema.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace memorial::query {

template <typename Type> struct is_typed_query : std::false_type {};

template <typename Schema, typename Current, typename Projection, typename Expression>
struct is_typed_query<typed_query<Schema, Current, Projection, Expression>> : std::true_type {};

template <typename Type>
concept Query = is_typed_query<std::remove_cvref_t<Type>>::value;

template <typename NodeOrTag> struct query_node_tag {
    using type = std::remove_cvref_t<NodeOrTag>;
};

template <NodeSpec Node> struct query_node_tag<Node> {
    using type = typename Node::tag;
};

template <typename NodeOrTag> using query_node_tag_t = typename query_node_tag<NodeOrTag>::type;

template <typename Type> struct is_property_key_token : std::false_type {};

template <meta::fixed_string Key>
struct is_property_key_token<property_key_token<Key>> : std::true_type {};

template <meta::fixed_string Key, typename Projection> struct projection_contains;

template <meta::fixed_string Key> struct projection_contains<Key, void> : std::false_type {};

template <meta::fixed_string Key, typename... Tokens>
struct projection_contains<Key, meta::type_list<Tokens...>>
    : std::bool_constant<(std::same_as<property_key_token<Key>, std::remove_cvref_t<Tokens>> ||
                          ...)> {};

template <typename Current, typename Projection, meta::fixed_string Key>
inline constexpr bool query_has_property_v =
    node_has_property_v<Current, Key> &&
    (std::same_as<Projection, void> || projection_contains<Key, Projection>::value);

template <GraphSchema Schema, typename Node> [[nodiscard]] constexpr auto from() {
    using node_tag = query_node_tag_t<Node>;
    static_assert(schema_has_node_v<node_tag, Schema>, "query source node is not in the schema");
    using expression = source_expr<node_tag>;
    return typed_query<Schema, schema_node_t<node_tag, Schema>, void, expression>{expression{}};
}

[[nodiscard]] constexpr auto active_at(timestamp value) noexcept { return active_at_expr{value}; }
[[nodiscard]] constexpr auto known_at(timestamp value) noexcept { return known_at_expr{value}; }
[[nodiscard]] constexpr auto in_worldline(worldline_id value) noexcept {
    return in_worldline_expr{value};
}

template <meta::fixed_string Key> inline constexpr property_ref<Key> prop{};

#define MEMORIAL_QUERY_COMPARISON(symbol, operation)                                               \
    template <meta::fixed_string Key, typename Value>                                              \
    [[nodiscard]] constexpr auto operator symbol(property_ref<Key> property, Value&& value) {      \
        return comparison_expr<property_ref<Key>, operation, std::decay_t<Value>>{                 \
            property, std::forward<Value>(value)};                                                 \
    }

MEMORIAL_QUERY_COMPARISON(>, greater_than_op)
MEMORIAL_QUERY_COMPARISON(>=, greater_equal_op)
MEMORIAL_QUERY_COMPARISON(<, less_than_op)
MEMORIAL_QUERY_COMPARISON(<=, less_equal_op)
MEMORIAL_QUERY_COMPARISON(==, equal_to_op)

#undef MEMORIAL_QUERY_COMPARISON

template <typename Predicate> [[nodiscard]] constexpr auto where(Predicate predicate) {
    return where_expr<std::decay_t<Predicate>>{std::move(predicate)};
}

template <typename Relation, typename Target> [[nodiscard]] constexpr auto traverse() noexcept {
    return traverse_expr<Relation, Target>{};
}

template <meta::fixed_string... Keys> [[nodiscard]] constexpr auto project() noexcept {
    static_assert(sizeof...(Keys) > 0, "query projection must contain at least one property");
    return project_expr<Keys...>{};
}

template <meta::fixed_string Key> struct property_order {
    [[nodiscard]] constexpr auto ascending() const noexcept {
        return order_expr<Key>{sort_direction::ascending};
    }

    [[nodiscard]] constexpr auto descending() const noexcept {
        return order_expr<Key>{sort_direction::descending};
    }
};

template <meta::fixed_string Key> inline constexpr property_order<Key> by{};

template <std::size_t Count, typename Order> [[nodiscard]] constexpr auto top_k(Order order) {
    static_assert(Count > 0, "top_k count must be greater than zero");
    return top_k_expr<Count, std::decay_t<Order>>{std::move(order)};
}

[[nodiscard]] constexpr auto limit(std::size_t count) noexcept { return limit_expr{count}; }

namespace detail {

template <typename Predicate> struct predicate_traits;

template <meta::fixed_string Key, typename Operator, typename Value>
struct predicate_traits<comparison_expr<property_ref<Key>, Operator, Value>> {
    static constexpr auto key = Key;
    using operator_type = Operator;
    using value_type = Value;
};

template <typename Predicate>
concept PropertyComparison = requires {
    predicate_traits<std::remove_cvref_t<Predicate>>::key;
    typename predicate_traits<std::remove_cvref_t<Predicate>>::value_type;
};

template <typename Value, typename Operator, typename Other>
concept ValidComparison =
    (std::same_as<Operator, greater_than_op> &&
     requires(const Value& left, const Other& right) {
         { left > right } -> std::convertible_to<bool>;
     }) ||
    (std::same_as<Operator, greater_equal_op> &&
     requires(const Value& left, const Other& right) {
         { left >= right } -> std::convertible_to<bool>;
     }) ||
    (std::same_as<Operator, less_than_op> &&
     requires(const Value& left, const Other& right) {
         { left < right } -> std::convertible_to<bool>;
     }) ||
    (std::same_as<Operator, less_equal_op> && requires(const Value& left, const Other& right) {
        { left <= right } -> std::convertible_to<bool>;
    }) || (std::same_as<Operator, equal_to_op> && requires(const Value& left, const Other& right) {
        { left == right } -> std::convertible_to<bool>;
    });

template <typename QueryType, typename Operation>
using appended_expression_t =
    pipeline_expr<typename QueryType::expression_type, std::remove_cvref_t<Operation>>;

} // namespace detail

template <Query QueryType>
[[nodiscard]] constexpr auto operator|(QueryType query, active_at_expr operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    using expression = detail::appended_expression_t<query_type, active_at_expr>;
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       typename query_type::projection, expression>{
        expression{query.expression(), operation}};
}

template <Query QueryType>
[[nodiscard]] constexpr auto operator|(QueryType query, known_at_expr operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    using expression = detail::appended_expression_t<query_type, known_at_expr>;
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       typename query_type::projection, expression>{
        expression{query.expression(), operation}};
}

template <Query QueryType>
[[nodiscard]] constexpr auto operator|(QueryType query, in_worldline_expr operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    using expression = detail::appended_expression_t<query_type, in_worldline_expr>;
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       typename query_type::projection, expression>{
        expression{query.expression(), operation}};
}

template <Query QueryType, detail::PropertyComparison Predicate>
[[nodiscard]] constexpr auto operator|(QueryType query, where_expr<Predicate> operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    using traits = detail::predicate_traits<std::remove_cvref_t<Predicate>>;
    constexpr auto key = traits::key;
    static_assert(query_has_property_v<typename query_type::current_node,
                                       typename query_type::projection, key>,
                  "query predicate property is unavailable on the current node");
    if constexpr (node_has_property_v<typename query_type::current_node, key>) {
        using property_type = node_property_value_t<typename query_type::current_node, traits::key>;
        static_assert(detail::ValidComparison<property_type, typename traits::operator_type,
                                              typename traits::value_type>,
                      "query predicate value cannot be compared with the property");
    }
    using expression = detail::appended_expression_t<query_type, where_expr<Predicate>>;
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       typename query_type::projection, expression>{
        expression{query.expression(), std::move(operation)}};
}

template <Query QueryType, typename Relation, typename Target>
[[nodiscard]] constexpr auto operator|(QueryType query, traverse_expr<Relation, Target> operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    using target_tag = query_node_tag_t<Target>;
    static_assert(std::same_as<typename query_type::projection, void>,
                  "query cannot traverse after projection");
    using current_tag = typename query_type::current_node::tag;
    static_assert(schema_has_node_v<target_tag, typename query_type::schema_type>,
                  "query traversal target is not in the schema");
    static_assert(
        schema_has_edge_v<current_tag, Relation, target_tag, typename query_type::schema_type>,
        "query traversal relation is not valid from the current node to the target");
    using expression = detail::appended_expression_t<query_type, traverse_expr<Relation, Target>>;
    return typed_query<typename query_type::schema_type,
                       schema_node_t<target_tag, typename query_type::schema_type>, void,
                       expression>{expression{query.expression(), operation}};
}

template <Query QueryType, meta::fixed_string... Keys>
[[nodiscard]] constexpr auto operator|(QueryType query, project_expr<Keys...> operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    static_assert(std::same_as<typename query_type::projection, void>,
                  "query cannot project more than once");
    static_assert((node_has_property_v<typename query_type::current_node, Keys> && ...),
                  "query projection contains a property unavailable on the current node");
    using projection = meta::type_list<property_key_token<Keys>...>;
    using expression = detail::appended_expression_t<query_type, project_expr<Keys...>>;
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       projection, expression>{expression{query.expression(), operation}};
}

template <Query QueryType, std::size_t Count, meta::fixed_string Key>
[[nodiscard]] constexpr auto operator|(QueryType query,
                                       top_k_expr<Count, order_expr<Key>> operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    static_assert(query_has_property_v<typename query_type::current_node,
                                       typename query_type::projection, Key>,
                  "query ordering property is unavailable on the current node");
    using expression = detail::appended_expression_t<query_type, decltype(operation)>;
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       typename query_type::projection, expression>{
        expression{query.expression(), operation}};
}

template <Query QueryType>
[[nodiscard]] constexpr auto operator|(QueryType query, limit_expr operation) {
    using query_type = std::remove_cvref_t<QueryType>;
    using expression = detail::appended_expression_t<query_type, limit_expr>;
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       typename query_type::projection, expression>{
        expression{query.expression(), operation}};
}

} // namespace memorial::query
