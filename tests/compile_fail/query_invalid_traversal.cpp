#include <memorial/query/dsl.hpp>
#include <memorial/schema/memorial_schema.hpp>

using namespace memorial::query;

int main() {
    const auto query = from<memorial::memorial_schema, memorial::domain::ghost_tag>() |
                       traverse<memorial::domain::causes_relation, memorial::domain::outcome_tag>();
    (void)query;
}
