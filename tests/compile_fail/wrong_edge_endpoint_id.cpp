#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/delta_store.hpp>

int main() {
    memorial::delta_store<memorial::memorial_schema> delta;
    (void)delta.append_edge<memorial::domain::experience_tag, memorial::domain::generates_relation,
                            memorial::domain::perception_tag>(
        memorial::node_id<memorial::domain::action_tag>{0U},
        memorial::node_id<memorial::domain::perception_tag>{0U}, 0.8);
}
