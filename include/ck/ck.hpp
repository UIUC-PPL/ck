#ifndef CK_CK_HPP
#define CK_CK_HPP

#include <ck/callback.hpp>
#include <ck/chare.hpp>
#include <ck/readonly.hpp>

namespace ck {
template <typename Base>
int chare_registrar<Base>::__register(void) {
  register_fn_t fn;
  if constexpr (is_main_chare_v<Base>) {
    fn = &(ck::chare<Base, main_chare>::__register);
  } else {
    fn = &(ck::index<Base>::__register);
  }
  auto& registry = CK_ACCESS_SINGLETON(chare_registry);
  auto ep = (int)registry.size();
  registry.emplace_back(fn);
  return ep;
}

inline void __register(void) {
  auto& chares = CK_ACCESS_SINGLETON(chare_registry);
  for (auto& chare : chares) {
    (*chare)();
  }

  auto& readonlies = CK_ACCESS_SINGLETON(readonly_registry);
  for (auto& readonly : readonlies) {
    (*readonly)();
  }
}
}  // namespace ck

#endif
