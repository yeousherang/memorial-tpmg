#pragma once

#include <memorial/query/dsl.hpp>

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace memorial::query {

template <typename Node, meta::fixed_string Key> struct required_column {
    using node = query_node_tag_t<Node>;
    static constexpr auto key = Key;
};

namespace detail {

template <typename Expression> struct optimizer_node_tag;

template <typename Node> struct optimizer_node_tag<source_expr<Node>> {
    using type = query_node_tag_t<Node>;
};

template <typename Previous, typename Operation>
struct optimizer_node_tag<pipeline_expr<Previous, Operation>> : optimizer_node_tag<Previous> {};

template <typename Previous, typename Relation, typename Target>
struct optimizer_node_tag<pipeline_expr<Previous, traverse_expr<Relation, Target>>> {
    using type = query_node_tag_t<Target>;
};

template <typename Type> struct is_selector : std::false_type {};
template <> struct is_selector<active_at_expr> : std::true_type {};
template <> struct is_selector<known_at_expr> : std::true_type {};
template <> struct is_selector<in_worldline_expr> : std::true_type {};

template <typename Type>
inline constexpr bool is_selector_v = is_selector<std::remove_cvref_t<Type>>::value;

template <typename Previous, typename Operation> struct append_optimizer {
    [[nodiscard]] static constexpr auto apply(Previous previous, Operation operation) {
        return pipeline_expr<Previous, Operation>{std::move(previous), std::move(operation)};
    }
};

template <typename Previous>
struct append_optimizer<pipeline_expr<Previous, limit_expr>, limit_expr> {
    [[nodiscard]] static constexpr auto apply(pipeline_expr<Previous, limit_expr> previous,
                                              limit_expr operation) {
        return pipeline_expr<Previous, limit_expr>{
            std::move(previous.previous),
            limit_expr{std::min(previous.operation.count, operation.count)}};
    }
};

template <typename Previous, typename First, typename Second>
struct append_optimizer<pipeline_expr<Previous, where_expr<First>>, where_expr<Second>> {
    [[nodiscard]] static constexpr auto apply(pipeline_expr<Previous, where_expr<First>> previous,
                                              where_expr<Second> operation) {
        using fused = fused_where_expr<First, Second>;
        return pipeline_expr<Previous, fused>{
            std::move(previous.previous), fused{std::tuple{std::move(previous.operation.predicate),
                                                           std::move(operation.predicate)}}};
    }
};

template <typename Previous, typename... Existing, typename Predicate>
struct append_optimizer<pipeline_expr<Previous, fused_where_expr<Existing...>>,
                        where_expr<Predicate>> {
    [[nodiscard]] static constexpr auto
    apply(pipeline_expr<Previous, fused_where_expr<Existing...>> previous,
          where_expr<Predicate> operation) {
        using fused = fused_where_expr<Existing..., Predicate>;
        return pipeline_expr<Previous, fused>{
            std::move(previous.previous),
            fused{std::tuple_cat(std::move(previous.operation.predicates),
                                 std::tuple{std::move(operation.predicate)})}};
    }
};

template <typename Previous, typename Selector, typename Predicate>
    requires is_selector_v<Selector>
struct append_optimizer<pipeline_expr<Previous, Selector>, where_expr<Predicate>> {
    [[nodiscard]] static constexpr auto apply(pipeline_expr<Previous, Selector> previous,
                                              where_expr<Predicate> operation) {
        auto pushed = append_optimizer<Previous, where_expr<Predicate>>::apply(
            std::move(previous.previous), std::move(operation));
        using pushed_type = decltype(pushed);
        return pipeline_expr<pushed_type, Selector>{std::move(pushed),
                                                    std::move(previous.operation)};
    }
};

template <typename Node> [[nodiscard]] constexpr auto normalize(const source_expr<Node>& source) {
    return source;
}

template <typename Previous, typename Operation>
[[nodiscard]] constexpr auto normalize(const pipeline_expr<Previous, Operation>& expression) {
    auto previous = normalize(expression.previous);
    using previous_type = decltype(previous);
    return append_optimizer<previous_type, Operation>::apply(std::move(previous),
                                                             expression.operation);
}

template <typename List> struct unique_types;

template <> struct unique_types<meta::type_list<>> {
    using type = meta::type_list<>;
};

template <typename First, typename... Rest> struct unique_types<meta::type_list<First, Rest...>> {
  private:
    using tail = typename unique_types<meta::type_list<Rest...>>::type;

  public:
    using type = std::conditional_t<meta::type_list_contains_v<First, tail>, tail,
                                    meta::type_list_push_front_t<First, tail>>;
};

template <typename Expression> struct required_columns;

template <typename Node> struct required_columns<source_expr<Node>> {
    using type = meta::type_list<>;
};

template <typename Previous, typename Operation>
struct required_columns<pipeline_expr<Previous, Operation>> : required_columns<Previous> {};

template <typename Previous, meta::fixed_string Key, typename Operator, typename Value>
struct required_columns<
    pipeline_expr<Previous, where_expr<comparison_expr<property_ref<Key>, Operator, Value>>>> {
    using type = meta::type_list_push_back_t<
        typename required_columns<Previous>::type,
        required_column<typename optimizer_node_tag<Previous>::type, Key>>;
};

template <typename Previous, typename... Predicates>
struct required_columns<pipeline_expr<Previous, fused_where_expr<Predicates...>>> {
    using node = typename optimizer_node_tag<Previous>::type;
    using type = meta::type_list_concat_t<
        typename required_columns<Previous>::type,
        meta::type_list<required_column<node, predicate_traits<Predicates>::key>...>>;
};

template <typename Previous, meta::fixed_string... Keys>
struct required_columns<pipeline_expr<Previous, project_expr<Keys...>>> {
    using node = typename optimizer_node_tag<Previous>::type;
    using type = meta::type_list_concat_t<typename required_columns<Previous>::type,
                                          meta::type_list<required_column<node, Keys>...>>;
};

template <typename Previous, std::size_t Count, meta::fixed_string Key>
struct required_columns<pipeline_expr<Previous, top_k_expr<Count, order_expr<Key>>>> {
    using type = meta::type_list_push_back_t<
        typename required_columns<Previous>::type,
        required_column<typename optimizer_node_tag<Previous>::type, Key>>;
};

template <typename Expression> struct expression_metrics;

template <typename Node> struct expression_metrics<source_expr<Node>> {
    static constexpr std::size_t depth = 1;
    static constexpr std::size_t filters = 0;
    static constexpr std::size_t traversals = 0;
};

template <typename Previous, typename Operation>
struct expression_metrics<pipeline_expr<Previous, Operation>> {
    static constexpr std::size_t depth = expression_metrics<Previous>::depth + 1;
    static constexpr std::size_t filters = expression_metrics<Previous>::filters;
    static constexpr std::size_t traversals = expression_metrics<Previous>::traversals;
};

template <typename Previous, typename Predicate>
struct expression_metrics<pipeline_expr<Previous, where_expr<Predicate>>>
    : expression_metrics<pipeline_expr<Previous, limit_expr>> {
    static constexpr std::size_t filters = expression_metrics<Previous>::filters + 1;
};

template <typename Previous, typename... Predicates>
struct expression_metrics<pipeline_expr<Previous, fused_where_expr<Predicates...>>>
    : expression_metrics<pipeline_expr<Previous, limit_expr>> {
    static constexpr std::size_t filters =
        expression_metrics<Previous>::filters + sizeof...(Predicates);
};

template <typename Previous, typename Relation, typename Target>
struct expression_metrics<pipeline_expr<Previous, traverse_expr<Relation, Target>>>
    : expression_metrics<pipeline_expr<Previous, limit_expr>> {
    static constexpr std::size_t traversals = expression_metrics<Previous>::traversals + 1;
};

} // namespace detail

template <typename Expression>
using required_columns_t =
    typename detail::unique_types<typename detail::required_columns<Expression>::type>::type;

template <Query QueryType> struct optimization_info {
    using query_type = std::remove_cvref_t<QueryType>;
    using expression_type = typename query_type::expression_type;
    using required_columns = required_columns_t<expression_type>;

    static constexpr std::size_t depth = detail::expression_metrics<expression_type>::depth;
    static constexpr std::size_t filter_count =
        detail::expression_metrics<expression_type>::filters;
    static constexpr std::size_t traversal_count =
        detail::expression_metrics<expression_type>::traversals;
};

template <Query QueryType> [[nodiscard]] constexpr auto optimize(const QueryType& query) {
    using query_type = std::remove_cvref_t<QueryType>;
    auto expression = detail::normalize(query.expression());
    using expression_type = decltype(expression);
    return typed_query<typename query_type::schema_type, typename query_type::current_node,
                       typename query_type::projection, expression_type>{std::move(expression)};
}

} // namespace memorial::query
