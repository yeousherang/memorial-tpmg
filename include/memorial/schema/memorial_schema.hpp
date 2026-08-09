#pragma once

#include <memorial/schema/graph_schema.hpp>

namespace memorial::domain {

// Layers
struct world_layer {};
struct perception_layer {};
struct cognition_layer {};
struct affect_body_layer {};
struct action_layer {};
struct memory_layer {};
struct latent_ghost_layer {};

// Node tags
struct experience_tag {};
struct perception_tag {};
struct thought_tag {};
struct belief_tag {};
struct goal_tag {};
struct emotion_tag {};
struct body_state_tag {};
struct decision_tag {};
struct action_tag {};
struct outcome_tag {};
struct memory_tag {};
struct ghost_tag {};

// Relation tags
struct temporal_next_relation {};
struct generates_relation {};
struct activates_relation {};
struct inhibits_relation {};
struct biases_relation {};
struct selects_relation {};
struct rejects_relation {};
struct causes_relation {};
struct encodes_relation {};
struct recalls_relation {};
struct reinterprets_relation {};
struct evidence_for_relation {};

// Shared static property definitions. Values remain runtime data.
using activation_property = property_spec<"activation", float>;
using confidence_property = property_spec<"confidence", float>;
using valence_property = property_spec<"valence", float>;
using existence_probability_property = property_spec<"existence_probability", double>;
using strength_property = property_spec<"strength", double>;

// Nodes
using experience_node = node_spec<experience_tag, world_layer, confidence_property>;
using perception_node = node_spec<perception_tag, perception_layer, confidence_property>;
using thought_node =
    node_spec<thought_tag, cognition_layer, activation_property, confidence_property>;
using belief_node = node_spec<belief_tag, cognition_layer, confidence_property>;
using goal_node = node_spec<goal_tag, cognition_layer, activation_property>;
using emotion_node =
    node_spec<emotion_tag, affect_body_layer, activation_property, valence_property>;
using body_state_node = node_spec<body_state_tag, affect_body_layer, activation_property>;
using decision_node = node_spec<decision_tag, action_layer, confidence_property>;
using action_node = node_spec<action_tag, action_layer, confidence_property>;
using outcome_node = node_spec<outcome_tag, world_layer, confidence_property>;
using memory_node = node_spec<memory_tag, memory_layer, activation_property, confidence_property>;
using ghost_node = node_spec<ghost_tag, latent_ghost_layer, activation_property,
                             confidence_property, existence_probability_property>;

// Representative edges for every required relation.
using experience_temporal_next_experience =
    edge_spec<experience_tag, temporal_next_relation, experience_tag,
              existence_probability_property>;
using experience_generates_perception =
    edge_spec<experience_tag, generates_relation, perception_tag, existence_probability_property>;
using perception_activates_thought = edge_spec<perception_tag, activates_relation, thought_tag,
                                               existence_probability_property, strength_property>;
using emotion_inhibits_thought = edge_spec<emotion_tag, inhibits_relation, thought_tag,
                                           existence_probability_property, strength_property>;
using ghost_biases_action = edge_spec<ghost_tag, biases_relation, action_tag,
                                      existence_probability_property, strength_property>;
using decision_selects_action =
    edge_spec<decision_tag, selects_relation, action_tag, existence_probability_property>;
using decision_rejects_action =
    edge_spec<decision_tag, rejects_relation, action_tag, existence_probability_property>;
using action_causes_outcome = edge_spec<action_tag, causes_relation, outcome_tag,
                                        existence_probability_property, strength_property>;
using outcome_encodes_memory = edge_spec<outcome_tag, encodes_relation, memory_tag,
                                         existence_probability_property, strength_property>;
using memory_recalls_experience = edge_spec<memory_tag, recalls_relation, experience_tag,
                                            existence_probability_property, strength_property>;
using memory_reinterprets_memory = edge_spec<memory_tag, reinterprets_relation, memory_tag,
                                             existence_probability_property, strength_property>;
using action_evidence_for_ghost = edge_spec<action_tag, evidence_for_relation, ghost_tag,
                                            existence_probability_property, strength_property>;

using memorial_relation_rules =
    relation_rules<relation_rule<temporal_next_relation, world_layer, world_layer>,
                   relation_rule<generates_relation, world_layer, perception_layer>,
                   relation_rule<activates_relation, perception_layer, cognition_layer>,
                   relation_rule<inhibits_relation, affect_body_layer, cognition_layer>,
                   relation_rule<biases_relation, latent_ghost_layer, action_layer>,
                   relation_rule<selects_relation, action_layer, action_layer>,
                   relation_rule<rejects_relation, action_layer, action_layer>,
                   relation_rule<causes_relation, action_layer, world_layer>,
                   relation_rule<encodes_relation, world_layer, memory_layer>,
                   relation_rule<recalls_relation, memory_layer, world_layer>,
                   relation_rule<reinterprets_relation, memory_layer, memory_layer>,
                   relation_rule<evidence_for_relation, action_layer, latent_ghost_layer>>;

using memorial_nodes = meta::type_list<experience_node, perception_node, thought_node, belief_node,
                                       goal_node, emotion_node, body_state_node, decision_node,
                                       action_node, outcome_node, memory_node, ghost_node>;

using memorial_edges =
    meta::type_list<experience_temporal_next_experience, experience_generates_perception,
                    perception_activates_thought, emotion_inhibits_thought, ghost_biases_action,
                    decision_selects_action, decision_rejects_action, action_causes_outcome,
                    outcome_encodes_memory, memory_recalls_experience, memory_reinterprets_memory,
                    action_evidence_for_ghost>;

using memorial_schema = graph_schema<memorial_nodes, memorial_edges, memorial_relation_rules>;

} // namespace memorial::domain

namespace memorial {

using memorial_schema = domain::memorial_schema;

} // namespace memorial
