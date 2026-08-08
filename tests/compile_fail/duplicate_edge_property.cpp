#include <memorial/schema/edge.hpp>

struct ghost_tag;
struct action_tag;
struct biases_relation;

using strength = memorial::property_spec<"strength", float>;
using duplicate_strength = memorial::property_spec<"strength", double>;
using invalid_edge =
    memorial::edge_spec<ghost_tag, biases_relation, action_tag, strength, duplicate_strength>;

static_assert(sizeof(invalid_edge) > 0);
