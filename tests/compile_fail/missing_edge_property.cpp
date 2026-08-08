#include <memorial/schema/edge.hpp>

struct ghost_tag;
struct action_tag;
struct biases_relation;

using biases = memorial::edge_spec<ghost_tag, biases_relation, action_tag,
                                   memorial::property_spec<"strength", float>>;
using missing_value = memorial::edge_property_value_t<biases, "probability">;

static_assert(sizeof(missing_value) > 0);
