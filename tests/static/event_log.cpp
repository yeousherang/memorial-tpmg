#include <memorial/runtime/event_log.hpp>

#include <concepts>
#include <span>
#include <string>
#include <type_traits>

using log_type = memorial::event_log<std::string>;
using event_type = log_type::event_type;

static_assert(memorial::StrongId<memorial::event_sequence>);
static_assert(memorial::EventPayload<std::string>);
static_assert(
    std::same_as<decltype(std::declval<const log_type&>().events()), std::span<const event_type>>);
static_assert(
    std::same_as<decltype(std::declval<const event_type&>().payload()), const std::string&>);
static_assert(
    !std::is_assignable_v<decltype(std::declval<const event_type&>().payload()), std::string>);

int main() {}
