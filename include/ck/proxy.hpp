#ifndef CK_PROXY_HPP
#define CK_PROXY_HPP

#include <ck/options.hpp>
#include <ck/registrar.hpp>

#define CPROXY_MEMBERS                          \
  using Index = index_of_t<Kind>;               \
  using local_t = Base;                         \
  using index_t = index<Base>;                  \
  using proxy_t = collection_proxy<Base, Kind>; \
  using element_t = element_proxy<Base, Kind>;  \
  using section_t = section_proxy<Base, Kind>;  \
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

template <typename Base, typename Kind = kind_of_t<Base>>
struct collection_proxy;

template <typename Base, typename Kind = kind_of_t<Base>>
struct element_proxy;

template <typename Base, typename Kind = kind_of_t<Base>>
struct section_proxy;

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

template <typename Base, auto Entry, typename Send, typename... Args>
void __grouplike_send(const Send &send, CkEntryOptions *opts, Args &&...args) {
  auto *msg = ck::pack(opts, std::forward<Args>(args)...);
  auto ep = index<Base>::template method_index<Entry>();
  send((CkArrayMessage *)msg, ep);
}
}  // namespace

template <typename Base, typename Kind>
struct section_proxy : public section_proxy_of_t<Kind> {
  using parent_t = section_proxy_of_t<Kind>;

  CPROXY_MEMBERS;

  template <typename... Args>
  section_proxy(Args &&...args) : parent_t(std::forward<Args>(args)...) {}

  template <typename... Args>
  static section_t create(Args &&...args) {
    return CkSectionID(std::forward<Args>(args)...);
  }

  // NOTE ( should this be called broadcast?       )
  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    if constexpr (std::is_same_v<CProxySection_ArrayElement, parent_t>) {
      __array_send<Base, Entry>(
          [&](CkArrayMessage *msg, int ep) {
            const_cast<section_t *>(this)->ckSend(msg, ep);
          },
          nullptr, std::forward<Args>(args)...);
    } else {
      static_assert(always_false<Args...>, "not implemented");
    }
  }

  element_t operator[](const Index &index) const {
    if constexpr (std::is_same_v<CProxySection_ArrayElement, parent_t>) {
      return element_t(this->ckGetArrayID(), index_view<Index>::encode(index),
                       this->ckDelegatedTo(), this->ckDelegatedPtr());
    } else {
      return element_t(this->ckGetGroupID(), index, this->ckDelegatedTo(),
                       this->ckDelegatedPtr());
    }
  }

  template <typename... Args>
  static void contribute(Args &&...args) {
    auto &sid = __info(args...);
    if constexpr (std::is_same_v<CProxySection_ArrayElement, parent_t>) {
      auto *arr = CProxy_CkArray(sid.get_aid()).ckLocalBranch();
      auto *grp = CProxy_CkMulticastMgr(arr->getmCastMgr()).ckLocalBranch();
      grp->contribute(std::forward<Args>(args)...);
    } else {
      static_assert(always_false<Args...>, "not implemented");
    }
  }
};

template <typename Base, typename Kind>
struct element_proxy : public element_proxy_of_t<Kind> {
  using parent_t = element_proxy_of_t<Kind>;

  CPROXY_MEMBERS;

  template <typename... Args>
  element_proxy(Args &&...args) : parent_t(std::forward<Args>(args)...) {}

  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    if constexpr (std::is_same_v<parent_t, CProxyElement_ArrayElement>) {
      __array_send<Base, Entry>(
          [&](CkArrayMessage *msg, int ep) { this->ckSend(msg, ep); }, nullptr,
          std::forward<Args>(args)...);
    } else {
      static_assert(always_false<Args...>, "not implemented");
    }
  }

  template <typename... Args>
  void insert(Args &&...args) const {
    if constexpr (std::is_same_v<parent_t, CProxy_ArrayElement>) {
      auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
      auto ctor = index<Base>::template constructor_index<Args...>();
      UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
      const_cast<element_t *>(this)->ckInsert((CkArrayMessage *)msg, ctor,
                                              CK_PE_ANY);
    } else {
      static_assert(always_false<Args...>, "not implemented");
    }
  }
};

template <typename Base, typename Kind>
struct collection_proxy : public collection_proxy_of_t<Kind> {
  static constexpr auto is_array = is_array_v<Kind>;
  using parent_t = collection_proxy_of_t<Kind>;

  CPROXY_MEMBERS;

  template <typename... Args>
  collection_proxy(Args &&...args) : parent_t(std::forward<Args>(args)...) {}

  template <typename... Args>
  static auto create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>()(
        std::forward<Args>(args)...);
  }

  template <auto Entry, typename... Args>
  void broadcast(Args &&...args) const {
    if constexpr (is_array) {
      __array_send<Base, Entry>(
          [&](CkArrayMessage *msg, int ep) { this->ckBroadcast(msg, ep); },
          nullptr, std::forward<Args>(args)...);
    } else {
      constexpr auto is_group = std::is_same_v<group, Kind>;
      constexpr auto &send =
          is_group ? CkBroadcastMsgBranch : CkBroadcastMsgNodeBranch;
      __grouplike_send<Base, Entry>(
          [&](CkMessage *msg, int ep) {
            send(ep, msg, this->ckGetGroupID(), 0);
          },
          nullptr, std::forward<Args>(args)...);
    }
  }

  // DRY violation with section proxy
  element_t operator[](const Index &index) const {
    if constexpr (is_array) {
      return element_t(this->ckGetArrayID(), index_view<Index>::encode(index),
                       this->ckDelegatedTo(), this->ckDelegatedPtr());
    } else {
      return element_t(this->ckGetGroupID(), index, this->ckDelegatedTo(),
                       this->ckDelegatedPtr());
    }
  }

  auto ckLocalBranch(void) const {
    if constexpr (is_array) {
      return parent_t::ckLocalBranch();
    } else {
      return (local_t *)CkLocalBranch(this->ckGetGroupID());
    }
  }
};

// alias for array proxies
template <typename Base, typename Index = index_of_t<kind_of_t<Base>>>
using array_proxy = collection_proxy<Base, array<Index>>;

// alias for group proxies
template <typename Base>
using group_proxy = collection_proxy<Base, group>;

// alias for nodegroup proxies
template <typename Base>
using nodegroup_proxy = collection_proxy<Base, nodegroup>;

namespace {
template <typename Base, typename Index, typename... Args>
array_proxy<Base, Index> __create_array(const constructor_options<Base> *opts,
                                        Args &&...args) {
  // pack the arguments into a message (with the entry options)
  auto *msg = ck::pack(const_cast<CkEntryOptions *>(&(opts->entry)),
                       std::forward<Args>(args)...);
  // retrieve the constructor's index
  auto ctor = index<Base>::template constructor_index<std::decay_t<Args>...>();
  // set the message type
  UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
  // create the array
  return CProxy_ArrayBase::ckCreateArray((CkArrayMessage *)msg, ctor,
                                         opts->array);
}

template <typename Base, typename Kind, typename... Args>
collection_proxy<Base, Kind> __create_grouplike(CkEntryOptions *opts,
                                                Args &&...args) {
  constexpr auto is_group = std::is_same_v<Kind, group>;
  constexpr auto msg_type = is_group ? BocInitMsg : NodeBocInitMsg;
  auto idx = index<Base>::__idx;
  auto ctor = index<Base>::template constructor_index<std::decay_t<Args>...>();
  auto *msg = ck::pack(opts, std::forward<Args>(args)...);
  UsrToEnv(msg)->setMsgtype(msg_type);
  if constexpr (is_group) {
    return CkCreateGroup(idx, ctor, msg);
  } else {
    return CkCreateNodeGroup(idx, ctor, msg);
  }
}
}  // namespace

// array creator with end of range given
template <typename Base, typename Index, typename... Ts>
struct creator<array_proxy<Base, Index>, Index, Ts...> {
  template <typename... Args>
  array_proxy<Base, Index> operator()(const Index &end, Args &&...args) const {
    constructor_options<Base> opts(end);
    return __create_array<Base, Index>(&opts,
                                       std::forward<decltype(args)>(args)...);
  }
};

// array creator with options given
template <typename Base, typename Index, typename... Ts>
struct creator<array_proxy<Base, Index>, constructor_options<Base>, Ts...> {
  template <typename... Args>
  array_proxy<Base, Index> operator()(const constructor_options<Base> &opts,
                                      Args &&...args) const {
    if constexpr ((sizeof...(Args) == 0) &&
                  !std::is_default_constructible_v<Base>) {
      auto *msg = CkAllocSysMsg(&(opts.entry));
      return CkCreateArray((CkArrayMessage *)msg, 0, opts.array);
    } else {
      return __create_array<Base, Index>(&opts, std::forward<Args>(args)...);
    }
  }
};

// array creator with nothing given
template <typename Base, typename Index>
struct creator<array_proxy<Base, Index>> {
  array_proxy<Base, Index> operator()(void) const {
    CkArrayOptions opts;
    return CProxy_ArrayBase::ckCreateEmptyArray(opts);
  }
};

// group creator with no options given
template <typename Base, typename... Ts>
struct creator<group_proxy<Base>, Ts...> {
  template <typename... Args>
  group_proxy<Base> operator()(Args &&...args) const {
    return __create_grouplike<Base, group>(nullptr,
                                           std::forward<Args>(args)...);
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