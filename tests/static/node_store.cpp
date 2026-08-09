#include <memorial/storage/node_store.hpp>

#include <concepts>
#include <cstddef>
#include <string>
#include <type_traits>

namespace {

struct person_tag {};
struct memory_layer {};

using person =
    memorial::node_spec<person_tag, memory_layer, memorial::property_spec<"name", std::string>,
                        memorial::property_spec<"confidence", float>>;
using store = memorial::node_store<person>;

template <typename Store, typename... Values>
concept AppendableWith =
    requires(Store& nodes, Values&&... values) { nodes.append(std::forward<Values>(values)...); };

static_assert(std::same_as<store::id_type, person::id_type>);
static_assert(std::same_as<decltype(std::declval<store&>().column<"name">().values),
                           std::vector<std::string>>);
static_assert(std::same_as<decltype(std::declval<const store&>().property<"confidence">(
                               std::declval<store::id_type>())),
                           memorial::result<std::reference_wrapper<const float>>>);
static_assert(AppendableWith<store, const char (&)[4], float>);
static_assert(!AppendableWith<store, float, const char (&)[4]>);

} // namespace

int main() {}
