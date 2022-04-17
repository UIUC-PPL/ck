#ifndef CK_CK_HPP
#define CK_CK_HPP

#include <ck/callback.hpp>
#include <ck/chare.hpp>
#include <ck/readonly.hpp>

namespace ck {
template <typename Base>
int main_chare_constructor(void) {
  if constexpr (std::is_constructible_v<Base, CkArgMsg*>) {
    return ck::index<Base>::template constructor_index<CkArgMsg*>();
  } else if constexpr (std::is_constructible_v<Base, int, char**>) {
    return ck::index<Base>::template constructor_index<int, char**>();
  } else {
    return ck::index<Base>::template constructor_index<>();
  }
}

template <typename Base>
int chare_registrar<Base>::__register(void) {
  auto& registry = CK_ACCESS_SINGLETON(chare_registry);
  auto ep = (int)registry.size();
  auto fn = &(ck::index<Base>::__register);
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
