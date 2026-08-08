#include <memorial/meta/type_map.hpp>

#include <concepts>
#include <type_traits>

namespace meta = memorial::meta;

struct name_key;
struct confidence_key;
struct missing_key;

using metadata = meta::type_map<meta::type_map_entry<name_key, const char*>,
                                meta::type_map_entry<confidence_key, float>>;

static_assert(meta::TypeMapEntry<meta::type_map_entry<name_key, const char*>>);
static_assert(!meta::TypeMapEntry<int>);
static_assert(meta::TypeMap<metadata>);
static_assert(!meta::TypeMap<int>);
static_assert(metadata::size == 2);
static_assert(meta::type_map_contains_v<name_key, metadata>);
static_assert(!meta::type_map_contains_v<missing_key, metadata>);
static_assert(std::same_as<meta::type_map_at_t<name_key, metadata>, const char*>);
static_assert(std::same_as<meta::type_map_at_t<confidence_key, metadata>, float>);

using numbers = meta::type_list<int, float, long, double>;
using integral_numbers = meta::type_list_filter_t<std::is_integral, numbers>;
static_assert(std::same_as<integral_numbers, meta::type_list<int, long>>);
static_assert(meta::type_list_any_of_v<std::is_floating_point, numbers>);
static_assert(!meta::type_list_all_of_v<std::is_integral, numbers>);
static_assert(meta::type_list_all_of_v<std::is_integral, meta::type_list<int, long>>);
static_assert(std::same_as<meta::type_list_find_if_t<std::is_floating_point, numbers>, float>);
static_assert(
    std::same_as<meta::type_list_find_if_t<std::is_pointer, numbers>, meta::type_list_not_found>);
static_assert(std::same_as<meta::type_list_find_if_or_t<std::is_pointer, numbers, void>, void>);

int main() {}
