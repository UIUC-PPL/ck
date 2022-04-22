#ifndef CK_TRAITS_HPP
#define CK_TRAITS_HPP

#include <ck/common.hpp>

namespace ck {

// determines whether a value is a bytes type (see PUPBytes)
template <typename T>
static constexpr auto is_bytes_v = PUP::as_bytes<T>::value;

template <typename Class, typename... Args>
using member_fn_t = void (Class::*)(Args...);

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

template <typename T>
using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;

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
