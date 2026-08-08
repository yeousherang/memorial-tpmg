#include <memorial/schema/edge.hpp>

struct ghost_tag;
struct action_tag;
struct biases_relation;

using invalid_edge = memorial::edge_spec<ghost_tag, biases_relation, action_tag, float>;

static_assert(sizeof(invalid_edge) > 0);
