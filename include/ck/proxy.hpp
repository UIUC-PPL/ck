#ifndef CK_PROXY_HPP
#define CK_PROXY_HPP

#include <ck/options.hpp>
#include <ck/registrar.hpp>
#include <variant>

#define CPROXY_MEMBERS                               \
  using Index = index_of_t<Kind>;                    \
  using kind_t = Kind;                               \
  using local_t = Base;                              \
  using index_t = index<Base>;                       \
  using proxy_t = collection_proxy<Base, Kind>;      \
  using element_t = element_proxy<Base, Kind>;       \
  using section_t = section_proxy<Base, Kind>;       \
  static constexpr auto is_array = is_array_v<Kind>; \
  static constexpr auto is_group = std::is_same_v<group, Kind>;

namespace ck {

// proxy for singleton chares
template <typename Base>
struct chare_proxy : public CProxy_Chare {
  using local_t = Base;
  using index_t = index<Base>;
  using proxy_t = chare_proxy<Base>;
  using element_t = chare_proxy<Base>;
  static constexpr auto is_array = false;

  chare_proxy(PUP::reconstruct) = delete;
  chare_proxy(CkMigrateMessage *) = delete;

  // TODO ( make this explicit! )
  template <typename... Args>
  chare_proxy(Args &&...args) : CProxy_Chare(std::forward<Args>(args)...) {}

  void send(CkMessage *msg, int ep, int flags) const {
    CkSendMsg(ep, msg, &(this->ckGetChareID()), flags);
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

template <typename Kind>
constexpr CkEnvelopeType message_type(bool is_elementlike) {
  if (is_array_v<Kind>) {
    return is_elementlike ? ForArrayEltMsg : ArrayBcastMsg;
  } else if (std::is_same_v<group, Kind>) {
    return is_elementlike ? ForBocMsg : BocBcastMsg;
  } else if (std::is_same_v<nodegroup, Kind>) {
    return ForNodeBocMsg;
  } else {
    return is_elementlike ? ForChareMsg : LAST_CK_ENVELOPE_TYPE;
  }
}

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

  void send(CkMessage *msg, int ep, int flags) const {
    if constexpr (is_array) {
      const_cast<section_t *>(this)->ckSend((CkArrayMessage *)msg, ep, flags);
    } else {
      constexpr auto &send_fn =
          is_group ? CkSendMsgBranchMulti : CkSendMsgNodeBranchMulti;
      for (auto i = 0; i < this->ckGetNumSections(); i++) {
        auto *copy = (i < (this->ckGetNumSections() - 1))
                         ? CkCopyMsg((void **)&msg)
                         : msg;
        send_fn(ep, copy, this->ckGetGroupIDn(i), this->ckGetNumElements(i),
                this->ckGetElements(i), flags);
      }
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

  void send(CkMessage *msg, int ep, int flags) const {
    if constexpr (is_array) {
      this->ckSend((CkArrayMessage *)msg, ep, flags);
    } else {
      constexpr auto &send_fn =
          is_group ? CkSendMsgBranch : CkSendMsgNodeBranch;
      send_fn(ep, msg, this->ckGetGroupPe(), this->ckGetGroupID(), flags);
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
    if constexpr (is_array) {
      return reinterpret_cast<local_t *>(parent_t::ckLocal());
    } else {
      auto pe = this->ckGetGroupPe();
      auto node = is_group ? CkNodeOf(pe) : pe;
      CkEnforceMsg(node == CkMyNode(), "non-local element");
      if constexpr (is_group) {
        auto *obj = CkLocalBranchOther(this->ckGetGroupID(), CkRankOf(pe));
        return reinterpret_cast<local_t *>(obj);
      } else {
        return reinterpret_cast<local_t *>(
            CkLocalNodeBranch(this->ckGetGroupID()));
      }
    }
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

  void send(CkMessage *msg, int ep, int flags) const {
    if constexpr (is_array) {
      this->ckBroadcast((CkArrayMessage *)msg, ep, flags);
    } else {
      constexpr auto &send_fn =
          is_group ? CkBroadcastMsgBranch : CkBroadcastMsgNodeBranch;
      send_fn(ep, msg, this->ckGetGroupID(), flags);
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

template <typename T>
struct is_elementlike : public std::false_type {};

template <typename Base>
struct is_elementlike<chare_proxy<Base>> : public std::true_type {};

template <typename Base, typename Kind>
struct is_elementlike<element_proxy<Base, Kind>> : public std::true_type {};

template <typename T>
constexpr auto is_elementlike_v = is_elementlike<T>::value;

template <typename T>
struct is_section : public std::false_type {};

template <typename T, typename Kind>
struct is_section<section_proxy<T, Kind>> : public std::true_type {};

template <typename T>
constexpr auto is_section_v = is_section<T>::value;
}  // namespace ck

#endif