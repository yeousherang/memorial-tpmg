#include <memorial/schema/relation_rules.hpp>

struct cognition_layer;
struct action_layer;
struct biases_relation;

using rule = memorial::relation_rule<biases_relation, cognition_layer, action_layer>;
using invalid_rules = memorial::relation_rules<rule, rule>;

static_assert(sizeof(invalid_rules) > 0);
