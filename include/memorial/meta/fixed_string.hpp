#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <string_view>

namespace memorial::meta {

// Owns the bytes of a null-terminated string literal so it can be used as a
// structural non-type template parameter. Size and indexing exclude no bytes;
// view() excludes only the final null terminator.
template <std::size_t Extent> struct fixed_string {
    static_assert(Extent > 0, "fixed_string requires a null terminator");

    char value[Extent]{};

    consteval fixed_string(const char (&literal)[Extent]) { std::ranges::copy(literal, value); }

    [[nodiscard]] static consteval std::size_t size() noexcept { return Extent - 1; }
    [[nodiscard]] static consteval bool empty() noexcept { return size() == 0; }

    [[nodiscard]] constexpr const char* data() const noexcept { return value; }

    [[nodiscard]] constexpr std::string_view view() const noexcept { return {value, size()}; }

    [[nodiscard]] constexpr operator std::string_view() const noexcept { return view(); }

    [[nodiscard]] constexpr char operator[](std::size_t index) const noexcept {
        return value[index];
    }
};

template <std::size_t Extent> fixed_string(const char (&)[Extent]) -> fixed_string<Extent>;

template <std::size_t LeftExtent, std::size_t RightExtent>
[[nodiscard]] constexpr bool operator==(const fixed_string<LeftExtent>& left,
                                        const fixed_string<RightExtent>& right) noexcept {
    return left.view() == right.view();
}

template <std::size_t LeftExtent, std::size_t RightExtent>
[[nodiscard]] constexpr auto operator<=>(const fixed_string<LeftExtent>& left,
                                         const fixed_string<RightExtent>& right) noexcept {
    return left.view() <=> right.view();
}

} // namespace memorial::meta
