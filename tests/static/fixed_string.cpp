#include <memorial/meta/fixed_string.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace meta = memorial::meta;

template <meta::fixed_string Name> struct named_entity {
    static constexpr auto name = Name;
};

constexpr meta::fixed_string empty_name{""};
constexpr meta::fixed_string memory_name{"memory"};
constexpr meta::fixed_string thought_name{"thought"};

static_assert(empty_name.empty());
static_assert(empty_name.size() == 0);
static_assert(empty_name.view().empty());
static_assert(empty_name.data()[0] == '\0');

static_assert(!memory_name.empty());
static_assert(memory_name.size() == 6);
static_assert(memory_name.view() == "memory");
static_assert(memory_name[0] == 'm');
static_assert(memory_name[memory_name.size()] == '\0');
static_assert(std::same_as<decltype(memory_name.view()), std::string_view>);
static_assert(noexcept(memory_name.view()));

static_assert(memory_name == meta::fixed_string{"memory"});
static_assert(memory_name != thought_name);
static_assert(memory_name < thought_name);

using memory_entity = named_entity<"memory">;
using another_memory_entity = named_entity<meta::fixed_string{"memory"}>;
static_assert(std::same_as<memory_entity, another_memory_entity>);
static_assert(memory_entity::name.view() == "memory");

static_assert(std::is_trivially_copyable_v<decltype(memory_name)>);
static_assert(std::is_standard_layout_v<decltype(memory_name)>);

int main() {}
