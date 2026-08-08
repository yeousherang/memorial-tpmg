#include <memorial/schema/node.hpp>

struct memory_tag;
struct cognitive_layer;

using memory_node =
    memorial::node_spec<memory_tag, cognitive_layer, memorial::property_spec<"confidence", float>>;
using missing_value = memorial::node_property_value_t<memory_node, "activation">;

static_assert(sizeof(missing_value) > 0);
