#include <memorial/schema/graph_schema.hpp>

struct thought_tag;
struct action_tag;
struct cognition_layer;
struct action_layer;
struct biases_relation;

using thought_node = memorial::node_spec<thought_tag, cognition_layer>;
using action_node = memorial::node_spec<action_tag, action_layer>;
using invalid_direction = memorial::edge_spec<action_tag, biases_relation, thought_tag>;
using rules = memorial::relation_rules<
    memorial::relation_rule<biases_relation, cognition_layer, action_layer>>;
using invalid_schema = memorial::graph_schema<memorial::meta::type_list<thought_node, action_node>,
                                              memorial::meta::type_list<invalid_direction>, rules>;

static_assert(sizeof(invalid_schema) > 0);
