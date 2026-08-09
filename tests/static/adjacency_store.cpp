#include <memorial/schema/memorial_schema.hpp>
#include <memorial/storage/adjacency_store.hpp>

#include <concepts>
#include <functional>
#include <span>

using edge = memorial::domain::experience_generates_perception;
using store = memorial::adjacency_store<edge>;

static_assert(memorial::StrongId<store::id_type>);
static_assert(
    std::same_as<store::source_id_type, memorial::node_id<memorial::domain::experience_tag>>);
static_assert(
    std::same_as<store::target_id_type, memorial::node_id<memorial::domain::perception_tag>>);
static_assert(std::same_as<decltype(std::declval<const store&>().outgoing(
                               std::declval<store::source_id_type>())),
                           std::span<const store::id_type>>);
static_assert(std::same_as<decltype(std::declval<const store&>().property<"existence_probability">(
                               std::declval<store::id_type>())),
                           memorial::result<std::reference_wrapper<const double>>>);

int main() {}
