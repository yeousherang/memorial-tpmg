#pragma once

#include <memorial/meta/type_list.hpp>

#include <concepts>
#include <type_traits>

namespace memorial {

template <typename Relation, typename SourceLayer, typename TargetLayer> struct relation_rule {
    using relation = Relation;
    using source_layer = SourceLayer;
    using target_layer = TargetLayer;
};

template <typename Type> struct is_relation_rule : std::false_type {};

template <typename Relation, typename SourceLayer, typename TargetLayer>
struct is_relation_rule<relation_rule<Relation, SourceLayer, TargetLayer>> : std::true_type {};

template <typename Type>
inline constexpr bool is_relation_rule_v = is_relation_rule<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept RelationRule = is_relation_rule_v<Type>;

template <RelationRule Rule> struct relation_rule_signature {
    using type = relation_rule<typename Rule::relation, typename Rule::source_layer,
                               typename Rule::target_layer>;
};

template <typename Relation, typename SourceLayer, typename TargetLayer>
struct relation_rule_matches {
    template <RelationRule Rule>
    struct predicate : std::bool_constant<std::same_as<Relation, typename Rule::relation> &&
                                          std::same_as<SourceLayer, typename Rule::source_layer> &&
                                          std::same_as<TargetLayer, typename Rule::target_layer>> {
    };
};

template <RelationRule... Rules> struct relation_rules {
    using rules = meta::type_list<Rules...>;
    using signatures = meta::type_list_transform_t<relation_rule_signature, rules>;

    static_assert(meta::type_list_is_unique_v<signatures>,
                  "relation rule signatures must be unique");

    template <typename Relation, typename SourceLayer, typename TargetLayer>
    static constexpr bool allows = meta::type_list_any_of_v<
        relation_rule_matches<Relation, SourceLayer, TargetLayer>::template predicate, rules>;
};

struct unrestricted_relation_rules {
    template <typename Relation, typename SourceLayer, typename TargetLayer>
    static constexpr bool allows = true;
};

template <typename Policy>
concept RelationRulePolicy = requires {
    { Policy::template allows<void, void, void> } -> std::convertible_to<bool>;
};

} // namespace memorial
