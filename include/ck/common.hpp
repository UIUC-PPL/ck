#ifndef CK_COMMON_HPP
#define CK_COMMON_HPP

#include <charm++.h>

namespace ck {
template <typename T>
void __dummy(const T&) {}

using register_fn_t = void (*)(void);

// TODO ( move definitions to object files? )
struct registry {
  // singleton instance of chare registry
  static std::vector<register_fn_t>& chares(void) {
    static std::vector<register_fn_t> instance;
    return instance;
  }

  // singleton instance of readonly registry
  static std::vector<register_fn_t>& readonlies(void) {
    static std::vector<register_fn_t> instance;
    return instance;
  }
};
}  // namespace ck

#endif
