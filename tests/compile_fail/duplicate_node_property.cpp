#include <memorial/schema/node.hpp>

struct memory_tag;
struct cognitive_layer;

using confidence = memorial::property_spec<"confidence", float>;
using duplicate_confidence = memorial::property_spec<"confidence", double>;
using invalid_node =
    memorial::node_spec<memory_tag, cognitive_layer, confidence, duplicate_confidence>;

static_assert(sizeof(invalid_node) > 0);
