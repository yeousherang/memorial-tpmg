#include <memorial/schema/node.hpp>

struct memory_tag;
struct cognitive_layer;

using invalid_node = memorial::node_spec<memory_tag, cognitive_layer, float>;

static_assert(sizeof(invalid_node) > 0);
