#include <memorial/id/strong_id.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>

struct memory_tag;
struct action_tag;

using memory_id = memorial::node_id<memory_tag>;
using action_id = memorial::node_id<action_tag>;
using wide_memory_id = memorial::strong_id<memory_tag, std::uint64_t>;

static_assert(memorial::IdValue<std::uint32_t>);
static_assert(!memorial::IdValue<std::int32_t>);
static_assert(!memorial::IdValue<bool>);
static_assert(memorial::StrongId<memory_id>);
static_assert(memorial::StrongId<const memory_id&>);
static_assert(!memorial::StrongId<std::uint32_t>);

static_assert(std::same_as<memory_id::tag_type, memory_tag>);
static_assert(std::same_as<memory_id::value_type, std::uint32_t>);
static_assert(std::same_as<wide_memory_id::value_type, std::uint64_t>);
static_assert(sizeof(memory_id) == sizeof(std::uint32_t));
static_assert(std::is_trivially_copyable_v<memory_id>);
static_assert(std::is_standard_layout_v<memory_id>);

static_assert(!std::convertible_to<std::uint32_t, memory_id>);
static_assert(!std::convertible_to<memory_id, std::uint32_t>);
static_assert(!std::convertible_to<memory_id, action_id>);
static_assert(!std::constructible_from<action_id, memory_id>);
static_assert(!std::equality_comparable_with<memory_id, action_id>);

constexpr memory_id first{1};
constexpr memory_id second{2};
constexpr memory_id missing;
static_assert(first.value() == 1);
static_assert(first.is_valid());
static_assert(static_cast<bool>(first));
static_assert(first < second);
static_assert(missing == memory_id::invalid());
static_assert(missing.value() == memory_id::invalid_value);
static_assert(!missing.is_valid());
static_assert(!static_cast<bool>(missing));

static_assert(std::regular<memory_id>);
static_assert(std::invocable<std::hash<memory_id>, memory_id>);

int main() {}
