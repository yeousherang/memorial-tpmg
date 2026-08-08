#include <memorial/schema/property.hpp>

using invalid_property = memorial::property_spec<"confidence", void>;

static_assert(sizeof(invalid_property) > 0);
