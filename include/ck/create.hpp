#ifndef CK_CREATE_HPP
#define CK_CREATE_HPP

#include <ck/proxy.hpp>

namespace ck {
// TODO ( enable passing PE as argument )
// TODO ( enable asynchronous (array?) creation )
// TODO ( extract options and pack arguments at this level )
template <typename Proxy, typename... Args>
auto create(Args&&... args) {
  return Proxy::template create<Args...>(std::forward<Args>(args)...);
}

template <typename Kind, typename Index, typename... Args>
auto insert(const element_proxy<Kind, array<Index>>& elt, Args&&... args) {
  return elt.template insert<Args...>(std::forward<Args>(args)...);
}
}  // namespace ck

#endif
