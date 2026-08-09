#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/delta_store.hpp>

struct unknown_tag {};

int main() {
    memorial::delta_store<memorial::memorial_schema> delta;
    (void)delta.nodes<unknown_tag>();
}
