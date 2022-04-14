#ifndef CK_PROXY_HPP
#define CK_PROXY_HPP

#include <ck/options.hpp>
#include <ck/registrar.hpp>

#define CPROXY_MEMBERS                          \
  using local_t = Base;                         \
  using index_t = index<Base>;                  \
  using proxy_t = array_proxy<Base, Index>;     \
  using element_t = element_proxy<Base, Index>; \
  using section_t = section_proxy<Base, Index>; \
  using array_index_t = Index;

namespace ck {

// creates a proxy with the given arguments
template <typename Proxy, typename... Args>
struct creator;

// proxy for singleton chares
template <typename Base>
struct chare_proxy : public CProxy_Chare {
  using local_t = Base;
  using index_t = index<Base>;
  using proxy_t = chare_proxy<Base>;
  using element_t = chare_proxy<Base>;

  template <typename... Args>
  chare_proxy(Args &&...args) : CProxy_Chare(std::forward<Args>(args)...) {}

  template <typename... Args>
  static chare_proxy<Base> create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>()(
        std::forward<Args>(args)...);
  }

  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ep = index<Base>::template method_index<Entry>();
    auto opts = 0;
    CkSendMsg(ep, msg, &ckGetChareID(), opts);
  }
};

template <typename Base, typename Index = array_index_of_t<Base>>
struct array_proxy;

template <typename Base, typename Index = array_index_of_t<Base>>
struct element_proxy;

namespace {
template <typename... Left, typename... Right>
CkSectionInfo &__info(Left &&..., CkSectionInfo &info, Right &&...) {
  return info;
}

template <typename Base, auto Entry, typename Send, typename... Args>
void __array_send(const Send &send, CkEntryOptions *opts, Args &&...args) {
  auto *msg = ck::pack(opts, std::forward<Args>(args)...);
  auto ep = index<Base>::template method_index<Entry>();
  UsrToEnv(msg)->setMsgtype(ForArrayEltMsg);
  ((CkArrayMessage *)msg)->array_setIfNotThere(CkArray_IfNotThere_buffer);
  send((CkArrayMessage *)msg, ep);
}
}  // namespace

template <typename Base, typename Index = array_index_of_t<Base>>
struct section_proxy : public CProxySection_ArrayBase {
  CPROXY_MEMBERS;

  template <typename... Args>
  section_proxy(Args &&...args)
      : CProxySection_ArrayBase(std::forward<Args>(args)...) {}

  template <typename... Args>
  static section_proxy<Base, Index> create(Args &&...args) {
    return CkSectionID(std::forward<Args>(args)...);
  }

  // NOTE ( should this be called broadcast?       )
  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    __array_send<Base, Entry>(
        [&](CkArrayMessage *msg, int ep) {
          const_cast<section_t *>(this)->ckSend(msg, ep);
        },
        nullptr, std::forward<Args>(args)...);
  }

  element_proxy<Base, Index> operator[](const Index &index) const {
    return element_proxy<Base, Index>(this->ckGetArrayID(),
                                      index_view<Index>::encode(index),
                                      CK_DELCTOR_CALL);
  }

  template <typename... Args>
  static void contribute(Args &&...args) {
    auto &sid = __info(args...);
    auto *arr = CProxy_CkArray(sid.get_aid()).ckLocalBranch();
    auto *grp = CProxy_CkMulticastMgr(arr->getmCastMgr()).ckLocalBranch();
    grp->contribute(std::forward<Args>(args)...);
  }
};

template <typename Base, typename Index>
struct element_proxy : public CProxyElement_ArrayBase {
  CPROXY_MEMBERS;

  template <typename... Args>
  element_proxy(Args &&...args)
      : CProxyElement_ArrayBase(std::forward<Args>(args)...) {}

  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    __array_send<Base, Entry>(
        [&](CkArrayMessage *msg, int ep) { this->ckSend(msg, ep); }, nullptr,
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void insert(Args &&...args) const {
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ctor = index<Base>::template constructor_index<Args...>();
    UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
    const_cast<element_t *>(this)->ckInsert((CkArrayMessage *)msg, ctor,
                                            CK_PE_ANY);
  }
};

template <typename Base, typename Index>
struct array_proxy : public CProxy_ArrayBase {
  CPROXY_MEMBERS;

  template <typename... Args>
  array_proxy(Args &&...args) : CProxy_ArrayBase(std::forward<Args>(args)...) {}

  template <typename... Args>
  static auto create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>()(
        std::forward<Args>(args)...);
  }

  template <auto Entry, typename... Args>
  void broadcast(Args &&...args) const {
    __array_send<Base, Entry>(
        [&](CkArrayMessage *msg, int ep) { this->ckBroadcast(msg, ep); },
        nullptr, std::forward<Args>(args)...);
  }

  element_proxy<Base, Index> operator[](const Index &index) const {
    return element_proxy<Base, Index>(this->ckGetArrayID(),
                                      index_view<Index>::encode(index),
                                      CK_DELCTOR_CALL);
  }
};

namespace {
template <typename Base, typename Index, typename... Args>
array_proxy<Base, Index> __create(const constructor_options<Base> *opts,
                                  Args &&...args) {
  // pack the arguments into a message (with the entry options)
  auto *msg = ck::pack(const_cast<CkEntryOptions *>(&(opts->entry)),
                       std::forward<Args>(args)...);
  // retrieve the constructor's index
  auto ctor = index<Base>::template constructor_index<Args...>();
  // set the message type
  UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
  // create the array
  return CProxy_ArrayBase::ckCreateArray((CkArrayMessage *)msg, ctor,
                                         opts->array);
}
}  // namespace

// creator with end of range given
template <typename Base, typename Index, typename... Args>
struct creator<array_proxy<Base, Index>, Index, Args...> {
  array_proxy<Base, Index> operator()(const Index &end, Args &&...args) const {
    constructor_options<Base> opts(end);
    return __create<Base, Index>(&opts, std::forward<Args>(args)...);
  }
};

// creator with options given
template <typename Base, typename Index, typename... Args>
struct creator<array_proxy<Base, Index>, constructor_options<Base>, Args...> {
  array_proxy<Base, Index> operator()(const constructor_options<Base> &opts,
                                      Args &&...args) const {
    if constexpr ((sizeof...(Args) == 0) &&
                  !std::is_default_constructible_v<Base>) {
      auto *msg = CkAllocSysMsg(&(opts.entry));
      return CkCreateArray((CkArrayMessage *)msg, 0, opts.array);
    } else {
      return __create<Base, Index>(&opts, std::forward<Args>(args)...);
    }
  }
};

// creator with nothing given
template <typename Base, typename Index>
struct creator<array_proxy<Base, Index>> {
  array_proxy<Base, Index> operator()(void) const {
    CkArrayOptions opts;
    return CProxy_ArrayBase::ckCreateEmptyArray(opts);
  }
};

// creator for singleton chares
template <typename Base, typename... Args>
struct creator<chare_proxy<Base>, Args...> {
  chare_proxy<Base> operator()(Args &&...args) const {
    CkChareID ret;
    auto *epopts = (CkEntryOptions *)nullptr;
    auto *msg = ck::pack(epopts, std::forward<Args>(args)...);
    auto ctor = index<Base>::template constructor_index<Args...>();
    CkCreateChare(index<Base>::__idx, ctor, msg, &ret, CK_PE_ANY);
    return ret;
  }
};
}  // namespace ck

#endif