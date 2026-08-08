#include <memorial/schema/property.hpp>

using invalid_property = memorial::property_spec<"", float>;

static_assert(sizeof(invalid_property) > 0);
