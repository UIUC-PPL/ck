#ifndef CK_PROXY_HPP
#define CK_PROXY_HPP

#include <ck/registrar.hpp>

#define CK_PROXY_FIELDS                         \
  using local_t = Base;                         \
  using index_t = index<Base>;                  \
  using proxy_t = array_proxy<Base, Index>;     \
  using element_t = element_proxy<Base, Index>; \
  using section_t = section_proxy<Base, Index>; \
  using array_index_t = Index;

namespace ck {

template <typename Base, typename Index = array_index_of_t<Base>>
struct array_proxy;

template <typename Base, typename Index = array_index_of_t<Base>>
struct element_proxy;

template <typename Base, typename Index = array_index_of_t<Base>>
struct section_proxy {};

template <typename Base, typename Index>
struct element_proxy : public CProxyElement_ArrayBase {
  CK_PROXY_FIELDS;

  template <typename... Args>
  element_proxy(Args &&...args)
      : CProxyElement_ArrayBase(std::forward<Args>(args)...) {}

  template <auto Entry, typename... Args>
  void send(Args &&...args) {
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ep = index<Base>::template method_index<Entry>();
    UsrToEnv(msg)->setMsgtype(ForArrayEltMsg);
    ((CkArrayMessage *)msg)->array_setIfNotThere(CkArray_IfNotThere_buffer);
    ckSend((CkArrayMessage *)msg, ep);
  }
};

template <typename Proxy, typename... Args>
struct creator;

template <typename Base, typename Index>
struct array_proxy : public CProxy_ArrayBase {
  CK_PROXY_FIELDS;

  template <typename... Args>
  array_proxy(Args &&...args) : CProxy_ArrayBase(std::forward<Args>(args)...) {}

  template <typename... Args>
  static array_proxy<Base, Index> create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>::create(
        std::forward<Args>(args)...);
  }

  // TODO ( fix DRY violation w/ element_proxy::send )
  template <auto Entry, typename... Args>
  void broadcast(Args &&...args) {
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ep = index<Base>::template method_index<Entry>();
    UsrToEnv(msg)->setMsgtype(ForArrayEltMsg);
    ((CkArrayMessage *)msg)->array_setIfNotThere(CkArray_IfNotThere_buffer);
    ckBroadcast((CkArrayMessage *)msg, ep);
  }

  element_proxy<Base, Index> operator[](const Index &index) const {
    return element_proxy<Base, Index>(this->ckGetArrayID(),
                                      index_view<Index>::encode(index));
  }
};

template <typename Base, typename Index, typename... Args>
struct creator<array_proxy<Base, Index>, Index, Args...> {
  static array_proxy<Base, Index> create(const Index &end, Args &&...args) {
    // TODO ( need to employ charmlite's encoding scheme here )
    CkArrayOptions opts(end);
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ctor = index<Base>::template constructor_index<Args...>();
    UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
    return CProxy_ArrayBase::ckCreateArray((CkArrayMessage *)msg, ctor, opts);
  }
};
}  // namespace ck

#endif