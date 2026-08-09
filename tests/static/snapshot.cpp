#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/snapshot.hpp>

#include <concepts>
#include <functional>
#include <type_traits>

using snapshot_type = memorial::snapshot<memorial::memorial_schema>;
using thought_id = memorial::node_id<memorial::domain::thought_tag>;

static_assert(std::copy_constructible<snapshot_type>);
static_assert(std::same_as<decltype(std::declval<const snapshot_type&>()
                                        .property<memorial::domain::thought_tag, "confidence">(
                                            std::declval<thought_id>())),
                           memorial::result<std::reference_wrapper<const float>>>);
static_assert(std::same_as<
              decltype(std::declval<const snapshot_type&>().contains(std::declval<thought_id>())),
              memorial::result<bool>>);

int main() {}
