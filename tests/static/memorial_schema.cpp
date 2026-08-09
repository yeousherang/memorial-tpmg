#include <memorial/schema/memorial_schema.hpp>

#include <concepts>

namespace domain = memorial::domain;

static_assert(memorial::GraphSchema<memorial::memorial_schema>);
static_assert(std::same_as<memorial::memorial_schema, domain::memorial_schema>);
static_assert(domain::memorial_schema::node_count == 12);
static_assert(domain::memorial_schema::edge_count == 12);

static_assert(std::same_as<domain::experience_node::layer, domain::world_layer>);
static_assert(std::same_as<domain::perception_node::layer, domain::perception_layer>);
static_assert(std::same_as<domain::thought_node::layer, domain::cognition_layer>);
static_assert(std::same_as<domain::belief_node::layer, domain::cognition_layer>);
static_assert(std::same_as<domain::goal_node::layer, domain::cognition_layer>);
static_assert(std::same_as<domain::emotion_node::layer, domain::affect_body_layer>);
static_assert(std::same_as<domain::body_state_node::layer, domain::affect_body_layer>);
static_assert(std::same_as<domain::decision_node::layer, domain::action_layer>);
static_assert(std::same_as<domain::action_node::layer, domain::action_layer>);
static_assert(std::same_as<domain::outcome_node::layer, domain::world_layer>);
static_assert(std::same_as<domain::memory_node::layer, domain::memory_layer>);
static_assert(std::same_as<domain::ghost_node::layer, domain::latent_ghost_layer>);

static_assert(memorial::node_has_property_v<domain::thought_node, "activation">);
static_assert(memorial::node_has_property_v<domain::ghost_node, "existence_probability">);
static_assert(memorial::edge_has_property_v<domain::ghost_biases_action, "strength">);

static_assert(memorial::ValidEdge<domain::memorial_schema, domain::experience_tag,
                                  domain::temporal_next_relation, domain::experience_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::experience_tag,
                                  domain::generates_relation, domain::perception_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::perception_tag,
                                  domain::activates_relation, domain::thought_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::emotion_tag,
                                  domain::inhibits_relation, domain::thought_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::ghost_tag,
                                  domain::biases_relation, domain::action_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::decision_tag,
                                  domain::selects_relation, domain::action_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::decision_tag,
                                  domain::rejects_relation, domain::action_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::action_tag,
                                  domain::causes_relation, domain::outcome_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::outcome_tag,
                                  domain::encodes_relation, domain::memory_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::memory_tag,
                                  domain::recalls_relation, domain::experience_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::memory_tag,
                                  domain::reinterprets_relation, domain::memory_tag>);
static_assert(memorial::ValidEdge<domain::memorial_schema, domain::action_tag,
                                  domain::evidence_for_relation, domain::ghost_tag>);
static_assert(!memorial::ValidEdge<domain::memorial_schema, domain::ghost_tag,
                                   domain::evidence_for_relation, domain::action_tag>);

int main() {}
