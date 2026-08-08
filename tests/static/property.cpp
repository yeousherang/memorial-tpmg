#include <memorial/schema/property.hpp>

#include <concepts>
#include <string>
#include <type_traits>

using activation = memorial::property_spec<"activation", float>;
using confidence = memorial::property_spec<"confidence", double>;
using label = memorial::property_spec<"label", std::string>;
using duplicate_confidence = memorial::property_spec<"confidence", int>;

static_assert(memorial::PropertyValue<float>);
static_assert(memorial::PropertyValue<std::string>);
static_assert(!memorial::PropertyValue<void>);
static_assert(!memorial::PropertyValue<float&>);
static_assert(!memorial::PropertyValue<const float>);
static_assert(!memorial::PropertyValue<float[2]>);

static_assert(memorial::PropertySpec<activation>);
static_assert(memorial::PropertySpec<const confidence&>);
static_assert(!memorial::PropertySpec<float>);
static_assert(activation::key.view() == "activation");
static_assert(std::same_as<activation::value_type, float>);

using properties = memorial::meta::type_list<activation, confidence, label>;
using duplicate_properties =
    memorial::meta::type_list<activation, confidence, duplicate_confidence>;

static_assert(memorial::property_keys_are_unique_v<properties>);
static_assert(!memorial::property_keys_are_unique_v<duplicate_properties>);
static_assert(memorial::has_property_v<"activation", properties>);
static_assert(!memorial::has_property_v<"missing", properties>);
static_assert(std::same_as<memorial::find_property_t<"confidence", properties>, confidence>);
static_assert(std::same_as<memorial::property_value_t<"label", properties>, std::string>);

int main() {}
