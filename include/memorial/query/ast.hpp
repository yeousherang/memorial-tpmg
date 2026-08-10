#pragma once

#include <memorial/meta/fixed_string.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/time.hpp>

#include <cstddef>
#include <utility>

namespace memorial::query {

enum class sort_direction { ascending, descending };

template <typename Node> struct source_expr {
    using node = Node;
};

struct active_at_expr {
    timestamp value;
};

struct known_at_expr {
    timestamp value;
};

struct in_worldline_expr {
    worldline_id value;
};

template <typename Predicate> struct where_expr {
    Predicate predicate;
};

template <typename Relation, typename Target> struct traverse_expr {
    using relation = Relation;
    using target = Target;
};

template <meta::fixed_string... Keys> struct project_expr {};

template <std::size_t Count, typename Order> struct top_k_expr {
    static constexpr std::size_t count = Count;
    Order order;
};

struct limit_expr {
    std::size_t count;
};

template <typename Previous, typename Operation> struct pipeline_expr {
    Previous previous;
    Operation operation;
};

template <meta::fixed_string Key> struct property_ref {
    static constexpr auto key = Key;
};

struct greater_than_op {};
struct greater_equal_op {};
struct less_than_op {};
struct less_equal_op {};
struct equal_to_op {};

template <typename Property, typename Operator, typename Value> struct comparison_expr {
    Property property;
    Value value;
};

template <meta::fixed_string Key> struct order_expr {
    static constexpr auto key = Key;
    sort_direction direction;
};

template <meta::fixed_string Key> struct property_key_token {
    static constexpr auto key = Key;
};

template <typename Schema, typename Current, typename Projection, typename Expression>
class typed_query {
  public:
    using schema_type = Schema;
    using current_node = Current;
    using projection = Projection;
    using expression_type = Expression;

    constexpr explicit typed_query(Expression expression) : expression_{std::move(expression)} {}

    [[nodiscard]] constexpr const Expression& expression() const noexcept { return expression_; }

  private:
    Expression expression_;
};

} // namespace memorial::query
