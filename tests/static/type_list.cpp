#include <memorial/meta/type_list.hpp>

#include <concepts>
#include <type_traits>

namespace meta = memorial::meta;

using empty = meta::type_list<>;
using types = meta::type_list<int, double, char>;
using duplicates = meta::type_list<int, double, int>;

static_assert(meta::TypeList<empty>);
static_assert(meta::TypeList<types>);
static_assert(!meta::TypeList<int>);

static_assert(meta::type_list_size_v<empty> == 0);
static_assert(meta::type_list_size_v<types> == 3);
static_assert(meta::type_list_contains_v<double, types>);
static_assert(!meta::type_list_contains_v<float, types>);
static_assert(meta::type_list_count_v<int, duplicates> == 2);
static_assert(meta::type_list_is_unique_v<empty>);
static_assert(meta::type_list_is_unique_v<types>);
static_assert(!meta::type_list_is_unique_v<duplicates>);

static_assert(std::same_as<meta::type_list_front_t<types>, int>);
static_assert(std::same_as<meta::type_list_at_t<1, types>, double>);
static_assert(meta::type_list_index_v<char, types> == 2);
static_assert(meta::type_list_index_v<float, types> == meta::type_list_npos);
static_assert(meta::type_list_index_v<float, empty> == meta::type_list_npos);

using combined =
    meta::type_list_concat_t<empty, meta::type_list<int>, meta::type_list<double, char>>;
static_assert(std::same_as<combined, types>);
static_assert(std::same_as<meta::type_list_push_front_t<float, types>,
                           meta::type_list<float, int, double, char>>);
static_assert(std::same_as<meta::type_list_push_back_t<types, float>,
                           meta::type_list<int, double, char, float>>);
static_assert(std::same_as<meta::type_list_transform_t<std::add_const, types>,
                           meta::type_list<const int, const double, const char>>);

int main() {}
