#ifndef CK_TRAITS_HPP
#define CK_TRAITS_HPP

#include <ck/common.hpp>

namespace ck {

template <typename T, typename Enable = void>
struct is_bytes : public PUP::as_bytes<T> {};

// this is generally assumed in this codebase
template <>
struct is_bytes<void> : public std::false_type {};

template <typename T>
struct is_bytes_span : public std::false_type {};

template <typename T>
struct is_bytes_span<ck::span<T>> : public is_bytes<T> {};

// determines whether a value is a bytes type (see PUPBytes)
template <typename T>
static constexpr auto is_bytes_v = is_bytes<T>::value;

// determines whether any parameter pack types match the predicate
template <template <class...> class Predicate, typename... Ts>
struct exists;

template <template <class...> class Predicate>
struct exists<Predicate> : public std::false_type {};

template <template <class...> class Predicate, typename T, typename... Ts>
struct exists<Predicate, T, Ts...>
    : public std::conditional_t<Predicate<T>::value, std::true_type,
                                exists<Predicate, Ts...>> {};

template <typename... Ts>
static constexpr auto has_bytes_span_v = exists<is_bytes_span, Ts...>::value;

template <typename Class, typename... Args>
using member_fn_t = void (Class::*)(Args...);

template <typename T, typename U>
struct tuple_compatibility_helper : public std::false_type {};

template <>
struct tuple_compatibility_helper<std::tuple<>, std::tuple<>>
    : public std::true_type {};

template <typename T, typename... Ts, typename U, typename... Us>
struct tuple_compatibility_helper<std::tuple<T, Ts...>, std::tuple<U, Us...>>
    : public std::conditional_t<
          std::is_assignable_v<std::decay_t<T>&, std::decay_t<U>&&>,
          tuple_compatibility_helper<std::tuple<Ts...>, std::tuple<Us...>>,
          std::false_type> {};

template <auto Entry, typename... Args>
struct is_compatible;

template <typename Class, typename... Ts, member_fn_t<Class, Ts...> Member,
          typename... Us>
struct is_compatible<Member, Us...> {
  static constexpr auto value =
      (sizeof...(Ts) == sizeof...(Us)) &&
      tuple_compatibility_helper<std::tuple<Ts...>, std::tuple<Us...>>::value;
};

template <auto Entry, typename... Args>
constexpr auto is_compatible_v = is_compatible<Entry, Args...>::value;

namespace {
template <typename... Ts>
constexpr std::tuple<std::decay_t<Ts>...> decay_types(std::tuple<Ts...> const&);
}

template <class... T>
constexpr bool always_false = false;

// decay all the types within a tuple
template <typename T>
using decay_tuple_t = decltype(decay_types(std::declval<T>()));

namespace {
template <template <typename...> class Base, typename... Ts>
std::true_type is_base_of_template_impl(const Base<Ts...>*);

template <template <typename...> class Base>
std::false_type is_base_of_template_impl(...);
}  // namespace

template <template <typename...> class Base, typename Derived>
using is_base_of_template =
    decltype(is_base_of_template_impl<Base>(std::declval<Derived*>()));

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

using main_arguments_t = std::tuple<int, char**>;

template <typename... Ts>
constexpr auto is_message_v = is_message_impl<Ts...>();

template <typename T, typename Enable = void>
struct message_index_of {
  // TODO ( need to make this a wildcard to accept any message )
  static constexpr auto& value = CkMarshallMsg::__idx;
};

template <typename T>
struct message_index_of<
    T, std::enable_if_t<std::is_same_v<T, main_arguments_t> ||
                        std::is_same_v<T, std::tuple<CkArgMsg*>>>> {
  static constexpr auto& value = CMessage_CkArgMsg::__idx;
};

template <typename Message, typename... Ts>
struct message_index_of<std::tuple<Message*, Ts...>,
                        std::enable_if_t<is_message_v<Message> &&
                                         !std::is_same_v<Message, CkArgMsg>>> {
  static constexpr auto& value = std::remove_cv_t<Message>::__idx;
};

template <typename... Ts>
constexpr auto& message_index_of_v = message_index_of<std::tuple<Ts...>>::value;

template <auto Entry>
struct message_index;

template <typename Class, typename... Args, member_fn_t<Class, Args...> Entry>
struct message_index<Entry> {
  static constexpr auto& value = message_index_of_v<Args...>;
};

template <auto Entry>
constexpr auto& message_index_v = message_index<Entry>::value;

namespace {
template <typename Class, typename... Args>
std::tuple<Args...> __arguments(member_fn_t<Class, Args...>);
}  // namespace

template <auto Fn>
using method_arguments_t = decltype(__arguments(Fn));

template <auto First, typename... Rest>
constexpr auto get_first_v = First;

namespace {
template <typename T>
struct tag {
  using type = T;
};
}  // namespace

template <typename T, typename Enable = void>
struct get_last;

template <>
struct get_last<std::tuple<>> {
  using type = void;
};

template <typename... Ts>
struct get_last<std::tuple<Ts...>, std::enable_if_t<(sizeof...(Ts) >= 1)>> {
  // Use a fold-expression to fold the comma operator over the parameter pack.
  using type = typename decltype((tag<Ts>{}, ...))::type;
};

// returns the last type in a parameter pack
template <typename... Ts>
using get_last_t = typename get_last<std::tuple<Ts...>>::type;
}  // namespace ck

#endif
