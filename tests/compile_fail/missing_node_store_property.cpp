#include <memorial/storage/node_store.hpp>

struct tag {};
struct layer {};

using node = memorial::node_spec<tag, layer, memorial::property_spec<"name", int>>;

int main() {
    memorial::node_store<node> nodes;
    (void)nodes.column<"missing">();
}
