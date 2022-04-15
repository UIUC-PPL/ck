#ifndef CK_ATTRIBUTES_HPP
#define CK_ATTRIBUTES_HPP

#include <type_traits>

#define CK_ENTRY_ATTRIBUTE(class, name)          \
  namespace ck {                                 \
  template <>                                    \
  struct class<name> : public std::true_type {}; \
  }

#define CK_EXCLUSIVE_ENTRY(name) CK_ENTRY_ATTRIBUTE(is_exclusive, name)

#define CK_THREADED_ENTRY(name) CK_ENTRY_ATTRIBUTE(is_threaded, name)

namespace ck {
template <auto Entry>
struct is_exclusive : public std::false_type {};

template <auto Entry>
constexpr auto is_exclusive_v = is_exclusive<Entry>::value;

template <auto Entry>
struct is_threaded : public std::false_type {};

template <auto Entry>
constexpr auto is_threaded_v = is_threaded<Entry>::value;
}  // namespace ck

#endif