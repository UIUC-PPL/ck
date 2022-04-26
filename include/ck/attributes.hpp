#ifndef CK_ATTRIBUTES_HPP
#define CK_ATTRIBUTES_HPP

#include <ck/traits.hpp>

#define CK_ENTRY_ATTRIBUTE_DECLARE(name)        \
  template <auto Entry, typename Enable = void> \
  struct is_##name : public std::false_type {}; \
  template <auto Entry>                         \
  constexpr auto is_##name##_v = is_##name<Entry>::value;

#define CK_ENTRY_ASSIGN_ATTRIBUTE(name, class, ...)                  \
  namespace ck {                                                     \
  template <__VA_ARGS__>                                             \
  struct is_##class<CK_PP_UNPAREN(name)> : public std::true_type {}; \
  }

// can we condense this down to a one-liner?
#define CK_EXCLUSIVE_ENTRY(...) \
  CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), exclusive)
#define CK_EXPEDITED_ENTRY(...) \
  CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), expedited)
#define CK_IMMEDIATE_ENTRY(...) \
  CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), immediate)
#define CK_INLINE_ENTRY(...) CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), inline)
#define CK_LOCAL_ENTRY(...) CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), local)
#define CK_NOKEEP_ENTRY(...) CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), nokeep)
#define CK_NOTRACE_ENTRY(...) CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), notrace)
#define CK_THREADED_ENTRY(...) \
  CK_ENTRY_ASSIGN_ATTRIBUTE((__VA_ARGS__), threaded)

namespace ck {

CK_ENTRY_ATTRIBUTE_DECLARE(exclusive);
CK_ENTRY_ATTRIBUTE_DECLARE(expedited);
CK_ENTRY_ATTRIBUTE_DECLARE(immediate);
CK_ENTRY_ATTRIBUTE_DECLARE(inline);
CK_ENTRY_ATTRIBUTE_DECLARE(local);
CK_ENTRY_ATTRIBUTE_DECLARE(notrace);
CK_ENTRY_ATTRIBUTE_DECLARE(threaded);

// TODO (add appwork, memcritcal, etc. )

template <typename... Ts>
class attributes;

template <>
class attributes<> : public std::integral_constant<int, 0> {};

template <typename T, typename... Ts>
class attributes<T, Ts...>
    : public std::integral_constant<int,
                                    (T::value | attributes<Ts...>::value)> {};

class Expedited : public std::integral_constant<int, CK_MSG_EXPEDITED> {};
class Immediate : public std::integral_constant<int, CK_MSG_IMMEDIATE> {};
class Inline : public std::integral_constant<int, CK_MSG_INLINE> {};
class Local : public std::integral_constant<int, 0> {};
class NoTrace : public std::integral_constant<int, CK_MSG_LB_NOTRACE> {};

template <auto Entry>
struct is_nokeep;

// most entry methods that use marshaling are automatically nokeep
template <typename Class, typename... Args, member_fn_t<Class, Args...> Entry>
struct is_nokeep<Entry> {
  static constexpr auto value = !(is_message_v<std::decay_t<Args>...> ||
                                  has_bytes_span_v<std::decay_t<Args>...>);
};

template <auto Entry>
constexpr auto is_nokeep_v = is_nokeep<Entry>::value;

namespace {
template <auto Entry>
constexpr auto registration_flags_value(void) {
  auto flags = 0;
  flags |= CK_EP_INLINE * is_inline_v<Entry>;
  flags |= CK_EP_IMMEDIATE * is_immediate_v<Entry>;
  flags |= CK_EP_NOKEEP * is_nokeep_v<Entry>;
  flags |= CK_EP_TRACEDISABLE * is_notrace_v<Entry>;
  return flags;
}

template <auto Entry>
constexpr auto message_flags_value(void) {
  auto flags = 0;
  flags |= Inline::value * is_inline_v<Entry>;
  flags |= Immediate::value * is_immediate_v<Entry>;
  flags |= Expedited::value * is_expedited_v<Entry>;
  flags |= NoTrace::value * is_notrace_v<Entry>;
  return flags;
}
}  // namespace

// returns an entry method's flags for a ckSend-like call
template <auto Entry>
constexpr auto message_flags_v = message_flags_value<Entry>();

// returns an entry method's registration flags (given to CkRegisterEp)
template <auto Entry>
constexpr auto registration_flags_v = registration_flags_value<Entry>();

template <typename Attribute, typename T>
constexpr auto contains_attribute_v = (bool)(Attribute::value& T::value);

template <typename T>
constexpr auto contains_inline_v = contains_attribute_v<Inline, T>;

template <typename T>
constexpr auto contains_local_v = contains_attribute_v<Local, T>;
}  // namespace ck

#endif
