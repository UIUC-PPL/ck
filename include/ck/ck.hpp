#ifndef CK_CK_HPP
#define CK_CK_HPP

#include <ck/callback.hpp>
#include <ck/chare.hpp>
#include <ck/create.hpp>
#include <ck/extends.hpp>
#include <ck/pupable.hpp>
#include <ck/readonly.hpp>
#include <ck/reduction.hpp>
#include <ck/send.hpp>

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
  auto& reg = registry::chares();
  auto ep = (int)reg.size();
  auto fn = &(ck::index<Base>::__register);
  reg.emplace_back(fn);
  return ep;
}
}  // namespace ck

#endif
