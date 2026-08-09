#include <memorial/schema/graph_schema.hpp>

struct ghost_tag;
struct action_tag;
struct cognitive_layer;
struct behavioral_layer;
struct biases_relation;

using ghost_node = memorial::node_spec<ghost_tag, cognitive_layer>;
using action_node = memorial::node_spec<action_tag, behavioral_layer>;
using strength = memorial::property_spec<"strength", float>;
using first_edge = memorial::edge_spec<ghost_tag, biases_relation, action_tag>;
using second_edge = memorial::edge_spec<ghost_tag, biases_relation, action_tag, strength>;
using invalid_schema = memorial::graph_schema<memorial::meta::type_list<ghost_node, action_node>,
                                              memorial::meta::type_list<first_edge, second_edge>>;

static_assert(sizeof(invalid_schema) > 0);
