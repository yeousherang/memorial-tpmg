#include <memorial/meta/type_map.hpp>

struct existing_key;
struct missing_key;

using metadata = memorial::meta::type_map<memorial::meta::type_map_entry<existing_key, int>>;
using missing_value = memorial::meta::type_map_at_t<missing_key, metadata>;

static_assert(sizeof(missing_value) > 0);
