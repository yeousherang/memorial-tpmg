#include <memorial/schema/memorial_schema.hpp>

#include <iostream>

int main() {
    std::cout << "Memorial schema: " << memorial::memorial_schema::node_count << " node types, "
              << memorial::memorial_schema::edge_count << " edge types\n";
}
