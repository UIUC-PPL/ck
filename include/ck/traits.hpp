#ifndef CK_TRAITS_HPP
#define CK_TRAITS_HPP

#include <charm++.h>

namespace ck {
namespace {
template <typename... Ts>
constexpr std::tuple<std::decay_t<Ts>...> decay_types(
    std::tuple<Ts...> const &);
}

template <typename T>
using decay_tuple_t = decltype(decay_types(std::declval<T>()));

namespace {
template <template <typename...> class Base, typename... Ts>
std::true_type is_base_of_template_impl(const Base<Ts...> *);

template <template <typename...> class Base>
std::false_type is_base_of_template_impl(...);
}  // namespace

template <template <typename...> class Base, typename Derived>
using is_base_of_template =
    decltype(is_base_of_template_impl<Base>(std::declval<Derived *>()));

template <template <typename...> class Base, typename Derived>
constexpr auto is_base_of_template_v =
    is_base_of_template<Base, Derived>::value;
}  // namespace ck

#endif
