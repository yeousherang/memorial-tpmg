#include <memorial/schema/graph_schema.hpp>

struct ghost_tag;
struct action_tag;
struct cognitive_layer;
struct biases_relation;

using ghost_node = memorial::node_spec<ghost_tag, cognitive_layer>;
using edge_to_unknown_action = memorial::edge_spec<ghost_tag, biases_relation, action_tag>;
using invalid_schema = memorial::graph_schema<memorial::meta::type_list<ghost_node>,
                                              memorial::meta::type_list<edge_to_unknown_action>>;

static_assert(sizeof(invalid_schema) > 0);
