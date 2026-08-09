#include <memorial/schema/graph_schema.hpp>

#include <concepts>

struct thought_tag;
struct action_tag;
struct cognition_layer;
struct action_layer;
struct biases_relation;

using cognition_biases_action =
    memorial::relation_rule<biases_relation, cognition_layer, action_layer>;
using rules = memorial::relation_rules<cognition_biases_action>;

static_assert(memorial::RelationRule<cognition_biases_action>);
static_assert(!memorial::RelationRule<int>);
static_assert(memorial::RelationRulePolicy<rules>);
static_assert(memorial::RelationRulePolicy<memorial::unrestricted_relation_rules>);
static_assert(rules::allows<biases_relation, cognition_layer, action_layer>);
static_assert(!rules::allows<biases_relation, action_layer, cognition_layer>);
static_assert(memorial::unrestricted_relation_rules::allows<int, float, double>);

using thought_node = memorial::node_spec<thought_tag, cognition_layer>;
using action_node = memorial::node_spec<action_tag, action_layer>;
using biases_edge = memorial::edge_spec<thought_tag, biases_relation, action_tag>;
using schema = memorial::graph_schema<memorial::meta::type_list<thought_node, action_node>,
                                      memorial::meta::type_list<biases_edge>, rules>;

static_assert(memorial::GraphSchema<schema>);
static_assert(std::same_as<schema::relation_rules, rules>);
static_assert(memorial::ValidEdge<schema, thought_tag, biases_relation, action_tag>);

int main() {}
