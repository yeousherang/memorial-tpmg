#include <memorial/meta/type_map.hpp>

struct duplicate_key;

using invalid_map = memorial::meta::type_map<memorial::meta::type_map_entry<duplicate_key, int>,
                                             memorial::meta::type_map_entry<duplicate_key, double>>;

static_assert(sizeof(invalid_map) > 0);
