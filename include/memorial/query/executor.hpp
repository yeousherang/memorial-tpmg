#pragma once

#include <memorial/query/dsl.hpp>
#include <memorial/storage/snapshot.hpp>

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace memorial::query {

template <NodeSpec Node, typename Projection> struct projected_row;

template <NodeSpec Node, typename... Tokens>
struct projected_row<Node, meta::type_list<Tokens...>> {
    using node_type = Node;
    using id_type = typename node_type::id_type;
    using projection = meta::type_list<Tokens...>;

    id_type id;
    std::tuple<node_property_value_t<node_type, Tokens::key>...> values;

    template <meta::fixed_string Key>
        requires meta::type_list_contains_v<property_key_token<Key>, projection>
    [[nodiscard]] const auto& value() const noexcept {
        constexpr auto index = meta::type_list_index_v<property_key_token<Key>, projection>;
        return std::get<index>(values);
    }
};

namespace detail {

template <typename Expression> struct expression_node_tag;

template <typename Node> struct expression_node_tag<source_expr<Node>> {
    using type = query_node_tag_t<Node>;
};

template <typename Previous, typename Operation>
struct expression_node_tag<pipeline_expr<Previous, Operation>> : expression_node_tag<Previous> {};

template <typename Previous, typename Relation, typename Target>
struct expression_node_tag<pipeline_expr<Previous, traverse_expr<Relation, Target>>> {
    using type = query_node_tag_t<Target>;
};

template <typename Expression>
using expression_node_tag_t = typename expression_node_tag<Expression>::type;

template <typename Tag> using id_list = std::vector<node_id<Tag>>;

template <GraphSchema Schema, typename Tag>
[[nodiscard]] result<id_list<Tag>> evaluate(const snapshot<Schema>& graph,
                                            const source_expr<Tag>& expression);

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, active_at_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, known_at_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, in_worldline_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Previous, meta::fixed_string Key, typename Operator,
          typename Value>
[[nodiscard]] auto evaluate(
    const snapshot<Schema>& graph,
    const pipeline_expr<Previous, where_expr<comparison_expr<property_ref<Key>, Operator, Value>>>&
        expression) -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Previous, typename... Predicates>
[[nodiscard]] auto
evaluate(const snapshot<Schema>& graph,
         const pipeline_expr<Previous, fused_where_expr<Predicates...>>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Previous, typename Relation, typename Target>
[[nodiscard]] auto
evaluate(const snapshot<Schema>& graph,
         const pipeline_expr<Previous, traverse_expr<Relation, Target>>& expression)
    -> result<id_list<query_node_tag_t<Target>>>;

template <GraphSchema Schema, typename Previous, meta::fixed_string... Keys>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, project_expr<Keys...>>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, limit_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Previous, std::size_t Count, meta::fixed_string Key>
[[nodiscard]] auto
evaluate(const snapshot<Schema>& graph,
         const pipeline_expr<Previous, top_k_expr<Count, order_expr<Key>>>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>>;

template <GraphSchema Schema, typename Tag>
[[nodiscard]] result<id_list<Tag>> evaluate(const snapshot<Schema>& graph,
                                            const source_expr<Tag>&) {
    return graph.template source_candidates<Tag>();
}

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, active_at_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>> {
    if (expression.operation.value != graph.valid_at()) {
        return std::unexpected(graph_error{graph_errc::conflict,
                                           "query active_at does not match the supplied snapshot"});
    }
    return evaluate(graph, expression.previous);
}

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, known_at_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>> {
    if (expression.operation.value != graph.known_at()) {
        return std::unexpected(graph_error{graph_errc::conflict,
                                           "query known_at does not match the supplied snapshot"});
    }
    return evaluate(graph, expression.previous);
}

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, in_worldline_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>> {
    if (expression.operation.value != graph.worldline()) {
        return std::unexpected(graph_error{graph_errc::worldline_mismatch,
                                           "query worldline does not match the supplied snapshot"});
    }
    return evaluate(graph, expression.previous);
}

template <typename Left, typename Right>
[[nodiscard]] constexpr bool compare(const Left& left, const Right& right, greater_than_op) {
    return left > right;
}

template <typename Left, typename Right>
[[nodiscard]] constexpr bool compare(const Left& left, const Right& right, greater_equal_op) {
    return left >= right;
}

template <typename Left, typename Right>
[[nodiscard]] constexpr bool compare(const Left& left, const Right& right, less_than_op) {
    return left < right;
}

template <typename Left, typename Right>
[[nodiscard]] constexpr bool compare(const Left& left, const Right& right, less_equal_op) {
    return left <= right;
}

template <typename Left, typename Right>
[[nodiscard]] constexpr bool compare(const Left& left, const Right& right, equal_to_op) {
    return left == right;
}

template <typename Operator> struct comparison_predicate {
    template <typename Left, typename Right>
    [[nodiscard]] constexpr bool operator()(const Left& left, const Right& right) const {
        return compare(left, right, Operator{});
    }
};

template <typename Id>
[[nodiscard]] std::vector<Id> retain_smaller_candidate_set(std::vector<Id> input,
                                                           std::vector<Id> indexed) {
    if (indexed.size() >= input.size()) {
        return input;
    }
    const std::unordered_set<Id> selected{indexed.begin(), indexed.end()};
    std::vector<Id> result;
    result.reserve(indexed.size());
    for (const auto id : input) {
        if (selected.contains(id)) {
            result.push_back(id);
        }
    }
    return result;
}

[[nodiscard]] constexpr bool should_use_property_index(std::size_t estimate,
                                                       std::size_t input_size) noexcept {
    return static_cast<double>(estimate) < static_cast<double>(input_size) * 0.8;
}

template <GraphSchema Schema, typename Tag, typename Predicate>
[[nodiscard]] result<bool> matches_predicate(const snapshot<Schema>& graph, node_id<Tag> id,
                                             const Predicate& predicate) {
    using traits = predicate_traits<Predicate>;
    const auto property = graph.template property<Tag, traits::key>(id);
    if (!property) {
        return std::unexpected(property.error());
    }
    return compare(property->get(), predicate.value, typename traits::operator_type{});
}

template <std::size_t Index = 0, GraphSchema Schema, typename Tag, typename... Predicates>
[[nodiscard]] result<bool> matches_all(const snapshot<Schema>& graph, node_id<Tag> id,
                                       const std::tuple<Predicates...>& predicates) {
    if constexpr (Index == sizeof...(Predicates)) {
        return true;
    } else {
        const auto matches = matches_predicate(graph, id, std::get<Index>(predicates));
        if (!matches || !*matches) {
            return matches;
        }
        return matches_all<Index + 1>(graph, id, predicates);
    }
}

template <std::size_t Index = 0, GraphSchema Schema, typename Tag, typename... Predicates>
[[nodiscard]] result<void> apply_index_candidates(const snapshot<Schema>& graph,
                                                  id_list<Tag>& input,
                                                  const std::tuple<Predicates...>& predicates) {
    if constexpr (Index == sizeof...(Predicates)) {
        return {};
    } else {
        const auto& predicate = std::get<Index>(predicates);
        using predicate_type = std::remove_cvref_t<decltype(predicate)>;
        using traits = predicate_traits<predicate_type>;
        const auto estimate = graph.template estimate_property_candidates<Tag, traits::key>(
            predicate.value, comparison_predicate<typename traits::operator_type>{});
        if (!estimate) {
            return std::unexpected(estimate.error());
        }
        if (should_use_property_index(*estimate, input.size())) {
            auto indexed = graph.template property_candidates<Tag, traits::key>(
                predicate.value, comparison_predicate<typename traits::operator_type>{});
            if (!indexed) {
                return std::unexpected(indexed.error());
            }
            input = retain_smaller_candidate_set(std::move(input), std::move(*indexed));
        }
        return apply_index_candidates<Index + 1>(graph, input, predicates);
    }
}

template <GraphSchema Schema, typename Previous, meta::fixed_string Key, typename Operator,
          typename Value>
[[nodiscard]] auto evaluate(
    const snapshot<Schema>& graph,
    const pipeline_expr<Previous, where_expr<comparison_expr<property_ref<Key>, Operator, Value>>>&
        expression) -> result<id_list<expression_node_tag_t<Previous>>> {
    using tag = expression_node_tag_t<Previous>;
    auto input = evaluate(graph, expression.previous);
    if (!input) {
        return std::unexpected(input.error());
    }
    const auto estimate = graph.template estimate_property_candidates<tag, Key>(
        expression.operation.predicate.value, comparison_predicate<Operator>{});
    if (!estimate) {
        return std::unexpected(estimate.error());
    }
    if (should_use_property_index(*estimate, input->size())) {
        auto indexed = graph.template property_candidates<tag, Key>(
            expression.operation.predicate.value, comparison_predicate<Operator>{});
        if (!indexed) {
            return std::unexpected(indexed.error());
        }
        *input = retain_smaller_candidate_set(std::move(*input), std::move(*indexed));
    }
    id_list<tag> output;
    output.reserve(input->size());
    for (const auto id : *input) {
        const auto property = graph.template property<tag, Key>(id);
        if (!property) {
            return std::unexpected(property.error());
        }
        if (compare(property->get(), expression.operation.predicate.value, Operator{})) {
            output.push_back(id);
        }
    }
    return output;
}

template <GraphSchema Schema, typename Previous, typename... Predicates>
[[nodiscard]] auto
evaluate(const snapshot<Schema>& graph,
         const pipeline_expr<Previous, fused_where_expr<Predicates...>>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>> {
    using tag = expression_node_tag_t<Previous>;
    auto input = evaluate(graph, expression.previous);
    if (!input) {
        return std::unexpected(input.error());
    }
    const auto indexed = apply_index_candidates(graph, *input, expression.operation.predicates);
    if (!indexed) {
        return std::unexpected(indexed.error());
    }
    id_list<tag> output;
    output.reserve(input->size());
    for (const auto id : *input) {
        const auto matches = matches_all(graph, id, expression.operation.predicates);
        if (!matches) {
            return std::unexpected(matches.error());
        }
        if (*matches) {
            output.push_back(id);
        }
    }
    return output;
}

template <GraphSchema Schema, typename Previous, typename Relation, typename Target>
[[nodiscard]] auto
evaluate(const snapshot<Schema>& graph,
         const pipeline_expr<Previous, traverse_expr<Relation, Target>>& expression)
    -> result<id_list<query_node_tag_t<Target>>> {
    using source = expression_node_tag_t<Previous>;
    using target = query_node_tag_t<Target>;
    auto input = evaluate(graph, expression.previous);
    if (!input) {
        return std::unexpected(input.error());
    }
    id_list<target> output;
    for (const auto source_id : *input) {
        const auto edges = graph.template outgoing<source, Relation, target>(source_id);
        if (!edges) {
            return std::unexpected(edges.error());
        }
        for (const auto edge : *edges) {
            const auto target_id = graph.template edge_target<source, Relation, target>(edge);
            if (!target_id) {
                return std::unexpected(target_id.error());
            }
            output.push_back(*target_id);
        }
    }
    return output;
}

template <GraphSchema Schema, typename Previous, meta::fixed_string... Keys>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, project_expr<Keys...>>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>> {
    return evaluate(graph, expression.previous);
}

template <GraphSchema Schema, typename Previous>
[[nodiscard]] auto evaluate(const snapshot<Schema>& graph,
                            const pipeline_expr<Previous, limit_expr>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>> {
    auto input = evaluate(graph, expression.previous);
    if (!input) {
        return std::unexpected(input.error());
    }
    if (input->size() > expression.operation.count) {
        input->resize(expression.operation.count);
    }
    return input;
}

template <GraphSchema Schema, typename Previous, std::size_t Count, meta::fixed_string Key>
[[nodiscard]] auto
evaluate(const snapshot<Schema>& graph,
         const pipeline_expr<Previous, top_k_expr<Count, order_expr<Key>>>& expression)
    -> result<id_list<expression_node_tag_t<Previous>>> {
    using tag = expression_node_tag_t<Previous>;
    using node = schema_node_t<tag, Schema>;
    using value = node_property_value_t<node, Key>;
    auto input = evaluate(graph, expression.previous);
    if (!input) {
        return std::unexpected(input.error());
    }
    std::vector<std::pair<node_id<tag>, value>> ranked;
    ranked.reserve(input->size());
    for (const auto id : *input) {
        const auto property = graph.template property<tag, Key>(id);
        if (!property) {
            return std::unexpected(property.error());
        }
        ranked.emplace_back(id, property->get());
    }
    const auto direction = expression.operation.order.direction;
    std::stable_sort(ranked.begin(), ranked.end(),
                     [direction](const auto& left, const auto& right) {
                         return direction == sort_direction::ascending ? left.second < right.second
                                                                       : left.second > right.second;
                     });
    if (ranked.size() > Count) {
        ranked.resize(Count);
    }
    input->clear();
    input->reserve(ranked.size());
    for (const auto& [id, ignored] : ranked) {
        static_cast<void>(ignored);
        input->push_back(id);
    }
    return input;
}

template <NodeSpec Node, typename Projection> struct execution_result {
    using type = std::vector<projected_row<Node, Projection>>;
};

template <NodeSpec Node> struct execution_result<Node, void> {
    using type = std::vector<typename Node::id_type>;
};

template <NodeSpec Node, typename Projection>
using execution_result_t = typename execution_result<Node, Projection>::type;

template <GraphSchema Schema, NodeSpec Node>
[[nodiscard]] result<std::tuple<>> read_projection(const snapshot<Schema>&, typename Node::id_type,
                                                   meta::type_list<>) {
    return std::tuple{};
}

template <GraphSchema Schema, NodeSpec Node, typename First, typename... Rest>
[[nodiscard]] auto read_projection(const snapshot<Schema>& graph, typename Node::id_type id,
                                   meta::type_list<First, Rest...>)
    -> result<std::tuple<node_property_value_t<Node, First::key>,
                         node_property_value_t<Node, Rest::key>...>> {
    const auto first = graph.template property<typename Node::tag, First::key>(id);
    if (!first) {
        return std::unexpected(first.error());
    }
    auto rest = read_projection<Schema, Node>(graph, id, meta::type_list<Rest...>{});
    if (!rest) {
        return std::unexpected(rest.error());
    }
    return std::tuple_cat(std::tuple{first->get()}, std::move(*rest));
}

template <GraphSchema Schema, NodeSpec Node, typename... Tokens>
[[nodiscard]] result<std::vector<projected_row<Node, meta::type_list<Tokens...>>>>
materialize(const snapshot<Schema>& graph, const std::vector<typename Node::id_type>& ids,
            meta::type_list<Tokens...>) {
    using row = projected_row<Node, meta::type_list<Tokens...>>;
    std::vector<row> rows;
    rows.reserve(ids.size());
    for (const auto id : ids) {
        auto values = read_projection<Schema, Node>(graph, id, meta::type_list<Tokens...>{});
        if (!values) {
            return std::unexpected(values.error());
        }
        rows.push_back(row{id, std::move(*values)});
    }
    return rows;
}

} // namespace detail

template <GraphSchema Schema, typename Current, typename Projection, typename Expression>
[[nodiscard]] auto execute(const snapshot<Schema>& graph,
                           const typed_query<Schema, Current, Projection, Expression>& query)
    -> result<detail::execution_result_t<Current, Projection>> {
    auto ids = detail::evaluate(graph, query.expression());
    if (!ids) {
        return std::unexpected(ids.error());
    }
    if constexpr (std::same_as<Projection, void>) {
        return *std::move(ids);
    } else {
        return detail::materialize<Schema, Current>(graph, *ids, Projection{});
    }
}

} // namespace memorial::query
