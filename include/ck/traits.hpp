#ifndef CK_TRAITS_HPP
#define CK_TRAITS_HPP

#include <charm++.h>

namespace ck {
namespace {
template <typename... Ts>
constexpr std::tuple<std::decay_t<Ts>...> decay_types(
    std::tuple<Ts...> const &);
}

template <class... T>
constexpr bool always_false = false;

// decay all the types within a tuple
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

// determine whether a class is derived from a templated base class
template <template <typename...> class Base, typename Derived>
constexpr auto is_base_of_template_v =
    is_base_of_template<Base, Derived>::value;

namespace {
template <typename... Ts>
constexpr bool is_message_impl(void) {
  if constexpr (sizeof...(Ts) == 1) {
    return std::is_base_of_v<CkMessage, std::remove_pointer_t<Ts>...>;
  } else {
    return false;
  }
}
}  // namespace

template <typename... Ts>
constexpr auto is_message_v = is_message_impl<Ts...>();

template <typename T>
using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;

template <class Base, class Kind>
struct chare;

template <typename T>
constexpr auto is_array_chare_v = is_base_of_template_v<chare, T>;
}  // namespace ck

#endif
