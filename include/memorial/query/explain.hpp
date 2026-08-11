#pragma once

#include <memorial/query/executor.hpp>
#include <memorial/query/optimizer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace memorial::query {

enum class plan_operation_kind {
    source_scan,
    valid_time_selector,
    transaction_time_selector,
    worldline_selector,
    predicate_filter,
    adjacency_traversal,
    projection,
    top_k,
    limit,
};

enum class index_kind {
    node_scan,
    adjacency,
    valid_time,
    transaction_time,
    worldline,
    property,
};

enum class kernel_kind {
    scalar_scan,
    simd_scan,
    sparse_index_lookup,
    single_thread_traversal,
    parallel_traversal,
};

struct required_column_info {
    std::size_t node_index;
    std::string key;

    friend bool operator==(const required_column_info&, const required_column_info&) = default;
};

struct index_candidate {
    index_kind kind;
    bool available;
    bool selected;
    std::string reason;
};

struct kernel_candidate {
    kernel_kind kind;
    bool selected;
    std::string reason;
};

struct query_plan {
    std::string signature;
    std::vector<plan_operation_kind> logical_ast;
    std::vector<plan_operation_kind> optimized_ast;
    std::vector<required_column_info> required_columns;
    std::vector<index_candidate> index_candidates;
    std::vector<kernel_candidate> kernel_candidates;
    std::size_t estimated_cardinality{};
    std::size_t actual_cardinality{};
};

namespace detail {

template <typename Operation> struct operation_kind;
template <> struct operation_kind<active_at_expr> {
    static constexpr auto value = plan_operation_kind::valid_time_selector;
};
template <> struct operation_kind<known_at_expr> {
    static constexpr auto value = plan_operation_kind::transaction_time_selector;
};
template <> struct operation_kind<in_worldline_expr> {
    static constexpr auto value = plan_operation_kind::worldline_selector;
};
template <typename Predicate> struct operation_kind<where_expr<Predicate>> {
    static constexpr auto value = plan_operation_kind::predicate_filter;
};
template <typename... Predicates> struct operation_kind<fused_where_expr<Predicates...>> {
    static constexpr auto value = plan_operation_kind::predicate_filter;
};
template <typename Relation, typename Target>
struct operation_kind<traverse_expr<Relation, Target>> {
    static constexpr auto value = plan_operation_kind::adjacency_traversal;
};
template <meta::fixed_string... Keys> struct operation_kind<project_expr<Keys...>> {
    static constexpr auto value = plan_operation_kind::projection;
};
template <std::size_t Count, typename Order> struct operation_kind<top_k_expr<Count, Order>> {
    static constexpr auto value = plan_operation_kind::top_k;
};
template <> struct operation_kind<limit_expr> {
    static constexpr auto value = plan_operation_kind::limit;
};

template <typename Node>
void append_operations(const source_expr<Node>&, std::vector<plan_operation_kind>& operations) {
    operations.push_back(plan_operation_kind::source_scan);
}

template <typename Previous, typename Operation>
void append_operations(const pipeline_expr<Previous, Operation>& expression,
                       std::vector<plan_operation_kind>& operations) {
    append_operations(expression.previous, operations);
    operations.push_back(operation_kind<Operation>::value);
}

template <typename Expression> struct source_node_tag;
template <typename Node> struct source_node_tag<source_expr<Node>> {
    using type = query_node_tag_t<Node>;
};
template <typename Previous, typename Operation>
struct source_node_tag<pipeline_expr<Previous, Operation>> : source_node_tag<Previous> {};

inline constexpr std::uint64_t signature_offset = 14695981039346656037ULL;
inline constexpr std::uint64_t signature_prime = 1099511628211ULL;

[[nodiscard]] constexpr std::uint64_t signature_mix(std::uint64_t hash,
                                                    std::uint64_t value) noexcept {
    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        hash ^= (value >> (byte * 8U)) & 0xFFU;
        hash *= signature_prime;
    }
    return hash;
}

[[nodiscard]] constexpr std::uint64_t signature_mix(std::uint64_t hash,
                                                    std::string_view value) noexcept {
    for (const auto character : value) {
        hash ^= static_cast<unsigned char>(character);
        hash *= signature_prime;
    }
    return hash;
}

template <GraphSchema Schema, typename Node>
[[nodiscard]] constexpr std::uint64_t expression_signature(source_expr<Node>) {
    constexpr auto index =
        meta::type_list_index_v<query_node_tag_t<Node>, typename Schema::node_tags>;
    return signature_mix(signature_mix(signature_offset, 1U), index);
}

template <typename Operation> struct operation_signature {
    [[nodiscard]] static constexpr std::uint64_t apply(std::uint64_t hash) {
        return signature_mix(hash, static_cast<std::uint64_t>(operation_kind<Operation>::value));
    }
};

template <typename Operator> struct comparison_signature;
template <>
struct comparison_signature<greater_than_op> : std::integral_constant<std::uint64_t, 1U> {};
template <>
struct comparison_signature<greater_equal_op> : std::integral_constant<std::uint64_t, 2U> {};
template <>
struct comparison_signature<less_than_op> : std::integral_constant<std::uint64_t, 3U> {};
template <>
struct comparison_signature<less_equal_op> : std::integral_constant<std::uint64_t, 4U> {};
template <> struct comparison_signature<equal_to_op> : std::integral_constant<std::uint64_t, 5U> {};

template <meta::fixed_string Key, typename Operator, typename Value>
struct operation_signature<where_expr<comparison_expr<property_ref<Key>, Operator, Value>>> {
    [[nodiscard]] static constexpr std::uint64_t apply(std::uint64_t hash) {
        hash =
            signature_mix(hash, static_cast<std::uint64_t>(plan_operation_kind::predicate_filter));
        hash = signature_mix(hash, Key.view());
        return signature_mix(hash, comparison_signature<Operator>::value);
    }
};

template <typename... Predicates> struct operation_signature<fused_where_expr<Predicates...>> {
    [[nodiscard]] static constexpr std::uint64_t apply(std::uint64_t hash) {
        hash =
            signature_mix(hash, static_cast<std::uint64_t>(plan_operation_kind::predicate_filter));
        ((hash = signature_mix(hash, predicate_traits<Predicates>::key.view()),
          hash = signature_mix(
              hash,
              comparison_signature<typename predicate_traits<Predicates>::operator_type>::value)),
         ...);
        return hash;
    }
};

template <meta::fixed_string... Keys> struct operation_signature<project_expr<Keys...>> {
    [[nodiscard]] static constexpr std::uint64_t apply(std::uint64_t hash) {
        hash = signature_mix(hash, static_cast<std::uint64_t>(plan_operation_kind::projection));
        ((hash = signature_mix(hash, Keys.view())), ...);
        return hash;
    }
};

template <std::size_t Count, meta::fixed_string Key>
struct operation_signature<top_k_expr<Count, order_expr<Key>>> {
    [[nodiscard]] static constexpr std::uint64_t apply(std::uint64_t hash) {
        hash = signature_mix(hash, static_cast<std::uint64_t>(plan_operation_kind::top_k));
        hash = signature_mix(hash, Key.view());
        return signature_mix(hash, Count);
    }
};

template <typename Relation, typename Target>
struct operation_signature<traverse_expr<Relation, Target>> {
    template <GraphSchema Schema, typename Source>
    [[nodiscard]] static constexpr std::uint64_t apply_with_schema(std::uint64_t hash) {
        using edge = schema_edge_t<Source, Relation, query_node_tag_t<Target>, Schema>;
        constexpr auto edge_index = meta::type_list_index_v<edge, typename Schema::edges>;
        return signature_mix(signature_mix(hash, static_cast<std::uint64_t>(
                                                     plan_operation_kind::adjacency_traversal)),
                             edge_index);
    }
};

template <GraphSchema Schema, typename Previous, typename Operation>
[[nodiscard]] constexpr std::uint64_t
expression_signature(const pipeline_expr<Previous, Operation>& expression) {
    auto hash = expression_signature<Schema>(expression.previous);
    if constexpr (requires {
                      operation_signature<Operation>::template apply_with_schema<
                          Schema, typename optimizer_node_tag<Previous>::type>(hash);
                  }) {
        using source = typename optimizer_node_tag<Previous>::type;
        return operation_signature<Operation>::template apply_with_schema<Schema, source>(hash);
    } else {
        return operation_signature<Operation>::apply(hash);
    }
}

[[nodiscard]] inline std::string signature_hex(std::uint64_t value) {
    constexpr std::array<char, 16> digits{'0', '1', '2', '3', '4', '5', '6', '7',
                                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::string result{"q-0000000000000000"};
    for (std::size_t index = 0; index < 16; ++index) {
        result[result.size() - 1U - index] = digits[value & 0xFU];
        value >>= 4U;
    }
    return result;
}

template <GraphSchema Schema, typename... Columns>
void append_columns(meta::type_list<Columns...>, std::vector<required_column_info>& output) {
    (output.push_back(required_column_info{
         meta::type_list_index_v<typename Columns::node, typename Schema::node_tags>,
         std::string{Columns::key.view()}}),
     ...);
}

} // namespace detail

template <GraphSchema Schema, typename Current, typename Projection, typename Expression>
[[nodiscard]] result<query_plan>
explain(const snapshot<Schema>& graph,
        const typed_query<Schema, Current, Projection, Expression>& query) {
    const auto optimized = optimize(query);
    using optimized_type = decltype(optimized);
    using source = typename detail::source_node_tag<Expression>::type;
    constexpr auto has_traversal = optimization_info<optimized_type>::traversal_count != 0U;
    constexpr auto has_filter = optimization_info<optimized_type>::filter_count != 0U;

    auto execution = execute(graph, optimized);
    if (!execution) {
        return std::unexpected(execution.error());
    }
    auto source_candidates = graph.template source_candidates<source>();
    if (!source_candidates) {
        return std::unexpected(source_candidates.error());
    }

    query_plan plan;
    plan.signature =
        detail::signature_hex(detail::expression_signature<Schema>(optimized.expression()));
    detail::append_operations(query.expression(), plan.logical_ast);
    detail::append_operations(optimized.expression(), plan.optimized_ast);
    detail::append_columns<Schema>(typename optimization_info<optimized_type>::required_columns{},
                                   plan.required_columns);
    plan.index_candidates = {
        {index_kind::node_scan, true, false,
         "temporal and worldline indexes provide narrower source candidates"},
        {index_kind::adjacency, true, has_traversal,
         has_traversal ? "selected for typed relation traversal" : "query has no traversal"},
        {index_kind::valid_time, true, true, "selected for snapshot valid-time visibility"},
        {index_kind::transaction_time, true, true,
         "selected for snapshot transaction-time visibility"},
        {index_kind::worldline, true, true, "selected for snapshot worldline visibility"},
        {index_kind::property, true, has_filter,
         has_filter ? "selected when its candidate set is narrower than the current input"
                    : "query has no property filter"},
    };
    plan.kernel_candidates = {
        {kernel_kind::scalar_scan, true, "selected for remaining filter and projection stages"},
        {kernel_kind::simd_scan, false, "SIMD kernel is not implemented"},
        {kernel_kind::sparse_index_lookup, true,
         has_filter ? "selected for temporal, worldline, and property lookup"
                    : "selected for temporal and worldline source lookup"},
        {kernel_kind::single_thread_traversal, has_traversal,
         has_traversal ? "selected for adjacency traversal" : "query has no traversal"},
        {kernel_kind::parallel_traversal, false, "parallel traversal is not implemented"},
    };
    plan.estimated_cardinality = source_candidates->size();
    plan.actual_cardinality = execution->size();
    return plan;
}

} // namespace memorial::query
