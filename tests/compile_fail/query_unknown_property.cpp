#include <memorial/query/dsl.hpp>
#include <memorial/schema/memorial_schema.hpp>

using namespace memorial::query;

int main() {
    const auto query = from<memorial::memorial_schema, memorial::domain::action_tag>() |
                       where(prop<"activation"> > 0.5F);
    (void)query;
}
