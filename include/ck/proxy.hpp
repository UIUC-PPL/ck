#ifndef CK_PROXY_HPP
#define CK_PROXY_HPP

#include <ck/options.hpp>
#include <ck/registrar.hpp>
#include <variant>

#define CPROXY_MEMBERS                               \
  using Index = index_of_t<Kind>;                    \
  using local_t = Base;                              \
  using index_t = index<Base>;                       \
  using proxy_t = collection_proxy<Base, Kind>;      \
  using element_t = element_proxy<Base, Kind>;       \
  using section_t = section_proxy<Base, Kind>;       \
  static constexpr auto is_array = is_array_v<Kind>; \
  static constexpr auto is_group = std::is_same_v<group, Kind>;

#define CPROXY_SEND_CHECK                        \
  static_assert(is_compatible_v<Entry, Args...>, \
                "arguments incompatible with entry method")

namespace ck {

// creates a proxy with the given arguments
template <typename Proxy, typename... Args>
struct creator;

namespace {
template <auto Entry, typename Proxy, typename... Args>
bool __try_inline_or_local(const Proxy *proxy, Args... args) {
  constexpr auto is_inline = is_inline_v<Entry>;
  if constexpr (is_inline || is_local_v<Entry>) {
    auto *local = proxy->ckLocal();
    if (local == nullptr) {
      CkEnforceMsg(is_inline, "local chare unavailable");
      return false;
    } else {
      CkCallstackPush(static_cast<Chare *>(local));
      (local->*Entry)(std::forward<Args>(args)...);
      CkCallstackPop(static_cast<Chare *>(local));
      return true;
    }
  } else {
    return false;
  }
}
}  // namespace

// proxy for singleton chares
template <typename Base>
struct chare_proxy : public CProxy_Chare {
  using local_t = Base;
  using index_t = index<Base>;
  using proxy_t = chare_proxy<Base>;
  using element_t = chare_proxy<Base>;

  chare_proxy(PUP::reconstruct) = delete;
  chare_proxy(CkMigrateMessage *) = delete;

  // TODO ( make this explicit! )
  template <typename... Args>
  chare_proxy(Args &&...args) : CProxy_Chare(std::forward<Args>(args)...) {}

  template <typename... Args>
  static chare_proxy<Base> create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>()(
        std::forward<Args>(args)...);
  }

  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    CPROXY_SEND_CHECK;

    if (__try_inline_or_local<Entry>(this, args...)) {
      return;
    }

    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ep = get_entry_index<Base, Entry>();
    auto opts = 0;
    CkSendMsg(ep, msg, &ckGetChareID(), opts);
  }

  auto ckLocal(void) const {
    auto &cid = this->ckGetChareID();
    return reinterpret_cast<local_t *>(CkLocalChare(&cid));
  }
};

template <typename Base, typename Kind = kind_of_t<Base>>
struct collection_proxy;

template <typename Base, typename Kind = kind_of_t<Base>>
struct element_proxy;

template <typename Base, typename Kind = kind_of_t<Base>>
struct section_proxy;

namespace {

// non-trivial helper function to detect when an CkEntryOptions* is
// given as the last argument of a send-call and move it to the
// appropriate position, based on: https://stackoverflow.com/q/31255890/
template <class Base, auto Entry,
          template <class, auto, class, class...> class Sender, class First,
          std::size_t... I0s, std::size_t... I1s, class... Ts>
void __send_with_options(
    const First &first,
    std::index_sequence<I0s...>,  // first args
    std::index_sequence<I1s...>,  // last args
    std::tuple<Ts..., CkEntryOptions *&&> args /* all args */) {
  Sender<Base, Entry, First, Ts...>()(std::get<I0s>(std::move(args))..., first,
                                      std::get<I1s>(std::move(args))...);
}

template <class Base, auto Entry,
          template <class, auto, class, class...> class Sender, class First,
          class... Ts>
void __send(const First &first, Ts &&...ts) {
  using last_t = get_last_t<Ts...>;
  if constexpr (std::is_same_v<CkEntryOptions *, std::decay_t<last_t>>) {
    __send_with_options<Base, Entry, Sender>(
        first, std::index_sequence<sizeof...(ts) - 1>{},  // put last first
        std::make_index_sequence<sizeof...(ts) - 1>{},    // put first last
        std::forward_as_tuple(std::forward<Ts>(ts)...));  // bundled args
  } else {
    Sender<Base, Entry, First, Ts...>()(nullptr, first,
                                        std::forward<Ts>(ts)...);
  }
}

template <typename... Left, typename... Right>
CkSectionInfo &__info(Left &&..., CkSectionInfo &info, Right &&...) {
  return info;
}

template <typename Proxy, typename Index>
auto __index(const Proxy *proxy, const Index &index) {
  using element_t = typename Proxy::element_t;

  if constexpr (Proxy::is_array) {
    return element_t(proxy->ckGetArrayID(), index_view<Index>::encode(index),
                     proxy->ckDelegatedTo(), proxy->ckDelegatedPtr());
  } else {
    return element_t(proxy->ckGetGroupID(), index, proxy->ckDelegatedTo(),
                     proxy->ckDelegatedPtr());
  }
}

template <typename Base, auto Entry, typename Send, typename... Args>
struct array_sender {
  CPROXY_SEND_CHECK;

  void operator()(CkEntryOptions *opts, const Send &send,
                  Args &&...args) const {
    auto *msg = ck::pack(opts, std::forward<Args>(args)...);
    auto ep = get_entry_index<Base, Entry>();
    UsrToEnv(msg)->setMsgtype(ForArrayEltMsg);
    ((CkArrayMessage *)msg)->array_setIfNotThere(CkArray_IfNotThere_buffer);
    send((CkArrayMessage *)msg, ep, message_flags_v<Entry>);
  }
};

template <typename Base, auto Entry, typename Send, typename... Args>
struct grouplike_sender {
  CPROXY_SEND_CHECK;

  void operator()(CkEntryOptions *opts, const Send &send,
                  Args &&...args) const {
    auto *msg = ck::pack(opts, std::forward<Args>(args)...);
    auto ep = get_entry_index<Base, Entry>();
    send((CkMessage *)msg, ep, message_flags_v<Entry>);
  }
};
}  // namespace

template <typename Base, typename Kind>
struct section_proxy : public section_proxy_of_t<Kind> {
  using parent_t = section_proxy_of_t<Kind>;

  CPROXY_MEMBERS;

  section_proxy(PUP::reconstruct) = delete;
  section_proxy(CkMigrateMessage *) = delete;

  // TODO ( make this explicit! )
  template <typename... Args>
  section_proxy(Args &&...args) : parent_t(std::forward<Args>(args)...) {}

  template <typename... Args>
  static auto create(Args &&...args) {
    return section_t(std::forward<Args>(args)...);
  }

  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    if constexpr (is_array) {
      __send<Base, Entry, array_sender>(
          [&](CkArrayMessage *msg, int ep, int flags) {
            const_cast<section_t *>(this)->ckSend(msg, ep, flags);
          },
          std::forward<Args>(args)...);
    } else {
      constexpr auto &send_fn =
          is_group ? CkSendMsgBranchMulti : CkSendMsgNodeBranchMulti;
      __send<Base, Entry, grouplike_sender>(
          [&](CkMessage *msg, int ep, int flags) {
            for (auto i = 0; i < this->ckGetNumSections(); i++) {
              auto *copy = (i < (this->ckGetNumSections() - 1))
                               ? CkCopyMsg((void **)&msg)
                               : msg;
              send_fn(ep, copy, this->ckGetGroupIDn(i),
                      this->ckGetNumElements(i), this->ckGetElements(i), flags);
            }
          },
          std::forward<Args>(args)...);
    }
  }

  template <typename T = CkArrayIndex>
  std::enable_if_t<(is_array && std::is_base_of_v<CkArrayIndex, T>), element_t>
  operator[](const T &index) const {
    return __index(this, index);
  }

  element_t operator[](const Index &index) const {
    return __index(this, index);
  }

  template <typename... Args>
  static std::enable_if_t<get_first_v<is_array, Args...>> contribute(
      Args &&...args) {
    auto &sid = __info(args...);
    auto *arr = CProxy_CkArray(sid.get_aid()).ckLocalBranch();
    auto *grp = CProxy_CkMulticastMgr(arr->getmCastMgr()).ckLocalBranch();
    grp->contribute(std::forward<Args>(args)...);
  }
};

template <typename Base, typename Kind>
struct element_proxy : public element_proxy_of_t<Kind> {
  using parent_t = element_proxy_of_t<Kind>;

  CPROXY_MEMBERS;

  element_proxy(PUP::reconstruct) = delete;
  element_proxy(CkMigrateMessage *) = delete;

  // TODO ( make this explicit! )
  template <typename... Args>
  element_proxy(Args &&...args) : parent_t(std::forward<Args>(args)...) {}

  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    if (__try_inline_or_local<Entry>(this, args...)) {
      return;
    }

    if constexpr (is_array) {
      __send<Base, Entry, array_sender>(
          [&](CkArrayMessage *msg, int ep, int flags) {
            this->ckSend(msg, ep, flags);
          },
          std::forward<Args>(args)...);
    } else {
      constexpr auto &send_fn =
          is_group ? CkSendMsgBranch : CkSendMsgNodeBranch;
      __send<Base, Entry, grouplike_sender>(
          [&](CkMessage *msg, int ep, int flags) {
            send_fn(ep, msg, this->ckGetGroupPe(), this->ckGetGroupID(), flags);
          },
          std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  std::enable_if_t<get_first_v<is_array, Args...>> insert(
      Args &&...args) const {
    auto *msg = ck::pack(nullptr, std::forward<Args>(args)...);
    auto ctor = index<Base>::template constructor_index<Args...>();
    UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
    const_cast<element_t *>(this)->ckInsert((CkArrayMessage *)msg, ctor,
                                            CK_PE_ANY);
  }

  auto ckLocal(void) const {
    return reinterpret_cast<local_t *>(parent_t::ckLocal());
  }
};

template <typename Base, typename Kind>
struct collection_proxy : public collection_proxy_of_t<Kind> {
  using parent_t = collection_proxy_of_t<Kind>;

  CPROXY_MEMBERS;

  collection_proxy(PUP::reconstruct) = delete;
  collection_proxy(CkMigrateMessage *) = delete;

  // TODO ( make this explicit! )
  template <typename... Args>
  collection_proxy(Args &&...args) : parent_t(std::forward<Args>(args)...) {}

  template <typename... Args>
  static auto create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>()(
        std::forward<Args>(args)...);
  }

  template <auto Entry, typename... Args>
  void send(Args &&...args) const {
    if constexpr (is_array) {
      __send<Base, Entry, array_sender>(
          [&](CkArrayMessage *msg, int ep, int flags) {
            this->ckBroadcast(msg, ep, flags);
          },
          std::forward<Args>(args)...);
    } else {
      constexpr auto &send_fn =
          is_group ? CkBroadcastMsgBranch : CkBroadcastMsgNodeBranch;
      __send<Base, Entry, grouplike_sender>(
          [&](CkMessage *msg, int ep, int flags) {
            send_fn(ep, msg, this->ckGetGroupID(), flags);
          },
          std::forward<Args>(args)...);
    }
  }

  template <typename T = CkArrayIndex>
  std::enable_if_t<(is_array && std::is_base_of_v<CkArrayIndex, T>), element_t>
  operator[](const T &index) const {
    return __index(this, index);
  }

  element_t operator[](const Index &index) const {
    return __index(this, index);
  }

  auto ckLocalBranch(void) const {
    if constexpr (is_array) {
      return parent_t::ckLocalBranch();
    } else if constexpr (is_group) {
      return reinterpret_cast<local_t *>(CkLocalBranch(this->ckGetGroupID()));
    } else {
      return reinterpret_cast<local_t *>(
          CkLocalNodeBranch(this->ckGetGroupID()));
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

template <typename T, typename Kind, typename Enable = void>
struct instance_proxy_of {
  using type = collection_proxy<T, Kind>;
};

template <typename T, typename Kind>
struct instance_proxy_of<
    T, Kind, std::enable_if_t<std::is_base_of_v<singleton_chare, Kind>>> {
  using type = chare_proxy<T>;
};

template <typename T, typename Kind = kind_of_t<T>>
using instance_proxy_of_t = typename instance_proxy_of<T, Kind>::type;

namespace {
template <typename Base, typename Index, typename Options, typename... Args>
array_proxy<Base, Index> __create_array(const Options &opts, Args &&...args) {
  // pack the arguments into a message (with the entry options)
  auto *msg = ck::pack(opts.e_opts, std::forward<Args>(args)...);
  // retrieve the constructor's index
  auto ctor = index<Base>::template constructor_index<std::decay_t<Args>...>();
  // set the message type
  UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
  // create the array
  return CProxy_ArrayBase::ckCreateArray((CkArrayMessage *)msg, ctor, opts);
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

// array creator with options given
template <typename Base, typename Index, typename... Ts>
struct creator<array_proxy<Base, Index>, Ts...> {
  using options_t = constructor_options<Base>;

 private:
  template <std::size_t... I0s, std::size_t... I1s, class... Args>
  static CkArrayID __create_with_options(std::index_sequence<I0s...>,
                                         std::index_sequence<I1s...>,
                                         std::tuple<Args...> args) {
    options_t opts(std::get<I0s>(std::move(args))...);
    return __create_array<Base, Index>(opts, std::get<I1s>(std::move(args))...);
  }

 public:
  template <typename... Args>
  array_proxy<Base, Index> operator()(Args &&...args) const {
    constexpr auto empty = sizeof...(Args) == 0;
    if constexpr (empty) {
      CkArrayOptions opts;
      return CProxy_ArrayBase::ckCreateEmptyArray(opts);
    } else {
      using last_t = get_last_t<Ts...>;
      static_assert(std::is_constructible_v<options_t, last_t>);
      return __create_with_options(
          std::index_sequence<sizeof...(Args) - 1>{},       // put last first
          std::make_index_sequence<sizeof...(Args) - 1>{},  // put first last
          std::forward_as_tuple(std::forward<Args>(args)...));
    }
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

template <typename Base, typename... Ts>
struct creator<nodegroup_proxy<Base>, Ts...> {
  template <typename... Args>
  nodegroup_proxy<Base> operator()(Args &&...args) const {
    return __create_grouplike<Base, nodegroup>(nullptr,
                                               std::forward<Args>(args)...);
  }
};

// creator for singleton chares
template <typename Base, typename... Ts>
struct creator<chare_proxy<Base>, Ts...> {
  template <typename... Args>
  chare_proxy<Base> operator()(Args &&...args) const {
    CkChareID ret;
    auto *epopts = (CkEntryOptions *)nullptr;
    auto *msg = ck::pack(epopts, std::forward<Args>(args)...);
    auto ctor = index<Base>::template constructor_index<Ts...>();
    CkCreateChare(index<Base>::__idx, ctor, msg, &ret, CK_PE_ANY);
    return ret;
  }
};
}  // namespace ck

#endif