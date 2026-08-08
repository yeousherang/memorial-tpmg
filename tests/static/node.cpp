#include <memorial/schema/node.hpp>

#include <concepts>
#include <cstdint>

struct memory_tag;
struct action_tag;
struct cognitive_layer;
struct behavioral_layer;

using activation = memorial::property_spec<"activation", float>;
using confidence = memorial::property_spec<"confidence", double>;

using memory_node = memorial::node_spec<memory_tag, cognitive_layer, activation, confidence>;
using action_node = memorial::node_spec<action_tag, behavioral_layer>;

static_assert(memorial::NodeSpec<memory_node>);
static_assert(memorial::NodeSpec<const memory_node&>);
static_assert(!memorial::NodeSpec<int>);
static_assert(std::same_as<memory_node::tag, memory_tag>);
static_assert(std::same_as<memory_node::layer, cognitive_layer>);
static_assert(
    std::same_as<memory_node::properties, memorial::meta::type_list<activation, confidence>>);
static_assert(std::same_as<memory_node::id_type, memorial::node_id<memory_tag>>);
static_assert(std::same_as<memory_node::id_type::value_type, std::uint32_t>);
static_assert(memory_node::property_count == 2);
static_assert(action_node::property_count == 0);

static_assert(memorial::node_has_property_v<memory_node, "activation">);
static_assert(!memorial::node_has_property_v<memory_node, "missing">);
static_assert(std::same_as<memorial::node_property_t<memory_node, "confidence">, confidence>);
static_assert(std::same_as<memorial::node_property_value_t<memory_node, "activation">, float>);

static_assert(!std::convertible_to<memory_node::id_type, action_node::id_type>);
static_assert(!std::equality_comparable_with<memory_node::id_type, action_node::id_type>);

int main() {}
