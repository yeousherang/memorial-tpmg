#include <memorial/schema/graph_schema.hpp>

struct memory_tag;
struct cognitive_layer;
struct narrative_layer;

using first_memory = memorial::node_spec<memory_tag, cognitive_layer>;
using second_memory = memorial::node_spec<memory_tag, narrative_layer>;
using invalid_schema =
    memorial::graph_schema<memorial::meta::type_list<first_memory, second_memory>,
                           memorial::meta::type_list<>>;

static_assert(sizeof(invalid_schema) > 0);
