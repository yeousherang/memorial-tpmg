#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/delta_store.hpp>

struct unknown_relation {};

int main() {
    memorial::delta_store<memorial::memorial_schema> delta;
    (void)delta.edges<memorial::domain::experience_tag, unknown_relation,
                      memorial::domain::perception_tag>();
}
