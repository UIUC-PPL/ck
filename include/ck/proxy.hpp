#ifndef CK_PROXY_HPP
#define CK_PROXY_HPP

#include <ck/registrar.hpp>

namespace ck {
template <typename Base>
struct element_proxy {};

template <typename Base>
struct section_proxy {};

template <typename T>
struct array_index_of;

template <typename Base>
using array_index_of_t = typename array_index_of<Base>::type;

template <typename Base, typename Index = array_index_of_t<Base>>
struct array_proxy;

template <typename Proxy, typename... Args>
struct creator;

template <typename Base, typename Index>
struct array_proxy : public CProxy_ArrayBase {
  using local_t = Base;
  using index_t = index<Base>;
  using proxy_t = array_proxy<Base, Index>;
  using element_t = element_proxy<Base>;
  using section_t = section_proxy<Base>;
  using array_index_t = Index;

  template <typename... Args>
  array_proxy(Args &&...args) : CProxy_ArrayBase(std::forward<Args>(args)...) {}

  template <typename... Args>
  static array_proxy<Base, Index> create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>::create(
        std::forward<Args>(args)...);
  }

  template <auto Entry, typename... Args>
  void broadcast(Args &&...args) {
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ep = index<Base>::template method_index<Entry>();
    UsrToEnv(msg)->setMsgtype(ForArrayEltMsg);
    ((CkArrayMessage *)msg)->array_setIfNotThere(CkArray_IfNotThere_buffer);
    ckBroadcast((CkArrayMessage *)msg, ep);
  }
};

template <typename Base, typename Index, typename... Args>
struct creator<array_proxy<Base, Index>, Index, Args...> {
  static array_proxy<Base, Index> create(const Index &end, Args &&...args) {
    CkArrayOptions opts(end);
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ctor = index<Base>::template constructor_index<Args...>();
    UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
    return CProxy_ArrayBase::ckCreateArray((CkArrayMessage *)msg, ctor, opts);
  }
};
}  // namespace ck

#endif