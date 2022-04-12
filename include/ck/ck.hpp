#ifndef CK_CK_HPP
#define CK_CK_HPP

#include <ck/chare.hpp>
#include <ck/main.hpp>

namespace ck {
template <typename T>
constexpr auto is_main_chare_v = is_base_of_template_v<main_chare, T>;

template <typename Base>
int chare_registrar<Base>::__register(void) {
  register_fn_t fn;
  if constexpr (is_main_chare_v<Base>) {
    fn = &(ck::main_chare<Base>::__register);
  } else {
    fn = &(ck::index<Base>::__register);
  }
  auto& registry = CK_ACCESS_SINGLETON(chare_registry);
  auto ep = (int)registry.size();
  registry.emplace_back(fn);
  return ep;
}
}  // namespace ck

extern "C" void CkRegisterMainModule(void) {
  auto& registry = CK_ACCESS_SINGLETON(ck::chare_registry);
  for (auto& chare : registry) {
    (*chare)();
  }
}

#endif
