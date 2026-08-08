#include <memorial/schema/edge.hpp>

#include <concepts>

struct ghost_tag;
struct action_tag;
struct biases_relation;

using probability = memorial::property_spec<"probability", double>;
using strength = memorial::property_spec<"strength", float>;

using ghost_biases_action =
    memorial::edge_spec<ghost_tag, biases_relation, action_tag, probability, strength>;
using action_biases_ghost = memorial::edge_spec<action_tag, biases_relation, ghost_tag>;

static_assert(memorial::EdgeSpec<ghost_biases_action>);
static_assert(memorial::EdgeSpec<const ghost_biases_action&>);
static_assert(!memorial::EdgeSpec<int>);
static_assert(std::same_as<ghost_biases_action::source, ghost_tag>);
static_assert(std::same_as<ghost_biases_action::relation, biases_relation>);
static_assert(std::same_as<ghost_biases_action::target, action_tag>);
static_assert(std::same_as<ghost_biases_action::properties,
                           memorial::meta::type_list<probability, strength>>);
static_assert(std::same_as<ghost_biases_action::source_id_type, memorial::node_id<ghost_tag>>);
static_assert(std::same_as<ghost_biases_action::target_id_type, memorial::node_id<action_tag>>);
static_assert(ghost_biases_action::property_count == 2);
static_assert(action_biases_ghost::property_count == 0);

static_assert(memorial::edge_has_property_v<ghost_biases_action, "probability">);
static_assert(!memorial::edge_has_property_v<ghost_biases_action, "missing">);
static_assert(std::same_as<memorial::edge_property_t<ghost_biases_action, "strength">, strength>);
static_assert(
    std::same_as<memorial::edge_property_value_t<ghost_biases_action, "probability">, double>);

static_assert(!std::same_as<ghost_biases_action, action_biases_ghost>);
static_assert(
    !std::convertible_to<ghost_biases_action::source_id_type, ghost_biases_action::target_id_type>);

int main() {}
