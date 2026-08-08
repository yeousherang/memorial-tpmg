#pragma once

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace memorial::meta {

inline constexpr std::size_t type_list_npos = static_cast<std::size_t>(-1);

template <typename... Types> struct type_list {
    static constexpr std::size_t size = sizeof...(Types);
};

template <typename Type> struct is_type_list : std::false_type {};

template <typename... Types> struct is_type_list<type_list<Types...>> : std::true_type {};

template <typename Type> inline constexpr bool is_type_list_v = is_type_list<Type>::value;

template <typename List>
concept TypeList = is_type_list_v<List>;

template <TypeList List> inline constexpr std::size_t type_list_size_v = List::size;

template <typename Sought, TypeList List> struct type_list_count;

template <typename Sought, typename... Types>
struct type_list_count<Sought, type_list<Types...>>
    : std::integral_constant<std::size_t, (std::size_t{0} + ... + std::is_same_v<Sought, Types>)> {
};

template <typename Sought, TypeList List>
inline constexpr std::size_t type_list_count_v = type_list_count<Sought, List>::value;

template <typename Sought, TypeList List>
inline constexpr bool type_list_contains_v = type_list_count_v<Sought, List> != 0;

template <TypeList List> struct type_list_is_unique;

template <typename... Types>
struct type_list_is_unique<type_list<Types...>>
    : std::bool_constant<((type_list_count_v<Types, type_list<Types...>> == 1) && ...)> {};

template <TypeList List>
inline constexpr bool type_list_is_unique_v = type_list_is_unique<List>::value;

template <std::size_t Index, TypeList List> struct type_list_at;

template <std::size_t Index, typename... Types>
    requires(Index < sizeof...(Types))
struct type_list_at<Index, type_list<Types...>> : std::tuple_element<Index, std::tuple<Types...>> {
};

template <std::size_t Index, TypeList List>
using type_list_at_t = typename type_list_at<Index, List>::type;

template <TypeList List> using type_list_front_t = type_list_at_t<0, List>;

template <typename Sought, typename... Types>
consteval std::size_t find_type_index(type_list<Types...>) {
    constexpr std::array<bool, sizeof...(Types)> matches{std::is_same_v<Sought, Types>...};
    for (std::size_t index = 0; index < sizeof...(Types); ++index) {
        if (matches[index]) {
            return index;
        }
    }
    return type_list_npos;
}

template <typename Sought, TypeList List>
inline constexpr std::size_t type_list_index_v = find_type_index<Sought>(List{});

template <TypeList... Lists> struct type_list_concat;

template <> struct type_list_concat<> {
    using type = type_list<>;
};

template <typename... Types> struct type_list_concat<type_list<Types...>> {
    using type = type_list<Types...>;
};

template <typename... Left, typename... Right, TypeList... Rest>
struct type_list_concat<type_list<Left...>, type_list<Right...>, Rest...>
    : type_list_concat<type_list<Left..., Right...>, Rest...> {};

template <TypeList... Lists> using type_list_concat_t = typename type_list_concat<Lists...>::type;

template <typename Type, TypeList List>
using type_list_push_front_t = type_list_concat_t<type_list<Type>, List>;

template <TypeList List, typename Type>
using type_list_push_back_t = type_list_concat_t<List, type_list<Type>>;

template <template <typename> typename Transform, TypeList List> struct type_list_transform;

template <template <typename> typename Transform, typename... Types>
struct type_list_transform<Transform, type_list<Types...>> {
    using type = type_list<typename Transform<Types>::type...>;
};

template <template <typename> typename Transform, TypeList List>
using type_list_transform_t = typename type_list_transform<Transform, List>::type;

} // namespace memorial::meta
