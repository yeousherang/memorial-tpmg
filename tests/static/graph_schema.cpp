#include <memorial/schema/graph_schema.hpp>

#include <concepts>

struct ghost_tag;
struct action_tag;
struct cognitive_layer;
struct behavioral_layer;
struct biases_relation;

using activation = memorial::property_spec<"activation", float>;
using strength = memorial::property_spec<"strength", double>;
using ghost_node = memorial::node_spec<ghost_tag, cognitive_layer, activation>;
using action_node = memorial::node_spec<action_tag, behavioral_layer>;
using biases_edge = memorial::edge_spec<ghost_tag, biases_relation, action_tag, strength>;

using schema = memorial::graph_schema<memorial::meta::type_list<ghost_node, action_node>,
                                      memorial::meta::type_list<biases_edge>>;
using empty_schema =
    memorial::graph_schema<memorial::meta::type_list<>, memorial::meta::type_list<>>;

static_assert(memorial::NodeSpecList<schema::nodes>);
static_assert(memorial::EdgeSpecList<schema::edges>);
static_assert(memorial::GraphSchema<schema>);
static_assert(memorial::GraphSchema<const schema&>);
static_assert(memorial::GraphSchema<empty_schema>);
static_assert(!memorial::GraphSchema<int>);
static_assert(schema::node_count == 2);
static_assert(schema::edge_count == 1);
static_assert(empty_schema::node_count == 0);
static_assert(empty_schema::edge_count == 0);

static_assert(schema::contains_node<ghost_tag>);
static_assert(!schema::contains_node<int>);
static_assert(memorial::schema_has_node_v<action_tag, schema>);
static_assert(std::same_as<memorial::schema_node_t<ghost_tag, schema>, ghost_node>);

static_assert(schema::contains_edge<ghost_tag, biases_relation, action_tag>);
static_assert(!schema::contains_edge<action_tag, biases_relation, ghost_tag>);
static_assert(memorial::schema_has_edge_v<ghost_tag, biases_relation, action_tag, schema>);
static_assert(std::same_as<memorial::schema_edge_t<ghost_tag, biases_relation, action_tag, schema>,
                           biases_edge>);
static_assert(memorial::ValidEdge<schema, ghost_tag, biases_relation, action_tag>);
static_assert(!memorial::ValidEdge<schema, action_tag, biases_relation, ghost_tag>);

int main() {}
