#ifndef CK_PROXY_HPP
#define CK_PROXY_HPP

#include <ck/registrar.hpp>

#define CPROXY_MEMBERS                          \
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
  CPROXY_MEMBERS;

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

// creates a proxy with the given arguments
template <typename Proxy, typename... Args>
struct creator;

template <typename Base, typename Index>
struct array_proxy : public CProxy_ArrayBase {
  CPROXY_MEMBERS;

  template <typename... Args>
  array_proxy(Args &&...args) : CProxy_ArrayBase(std::forward<Args>(args)...) {}

  template <typename... Args>
  static array_proxy<Base, Index> create(Args &&...args) {
    return creator<proxy_t, std::decay_t<Args>...>()(
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

// initialize options with the given options
template <typename Options, typename Index, typename Enable = void>
struct options_initializer {
  template <typename... Args>
  void operator()(Options &opts, Args &&...args) const {
    new (&opts) Options(std::forward<Args>(args)...);
  }
};

namespace {
template <typename Base, typename Index, typename... Sizes, typename... Args>
array_proxy<Base, Index> __create(std::tuple<Sizes...> sizes,
                                  std::tuple<Args...> args) {
  // set up all the options
  storage_t<CkArrayOptions> storage;
  auto *epopts = (CkEntryOptions *)nullptr;
  auto &aropts = reinterpret_cast<CkArrayOptions &>(storage);
  std::apply(
      [&](auto... ts) {
        // initialize the options with the given sizes
        options_initializer<CkArrayOptions, Index>()(
            aropts, std::forward<Sizes>(ts)...);
      },
      sizes);
  // pack the arguments into a message (with the entry options)
  auto *msg = std::apply(
      [&](auto... ts) { return ck::pack(epopts, std::forward(ts)...); }, args);
  // retrieve the constructor's index
  auto ctor = index<Base>::template constructor_index<Args...>();
  // set the message type
  UsrToEnv(msg)->setMsgtype(ArrayEltInitMsg);
  // create the array
  return CProxy_ArrayBase::ckCreateArray((CkArrayMessage *)msg, ctor, aropts);
}
}  // namespace

// creator with the end of range given
template <typename Base, typename Index, typename... Args>
struct creator<array_proxy<Base, Index>, Index, Args...> {
  array_proxy<Base, Index> operator()(const Index &end, Args &&...args) const {
    return __create<Base, Index>(std::forward_as_tuple(end),
                                 std::forward_as_tuple(args...));
  }
};

// creator with no sizes given
template <typename Base, typename Index, typename... Args>
struct creator<array_proxy<Base, Index>, Args...> {
  array_proxy<Base, Index> operator()(Args &&...args) const {
    return __create<Base, Index>(std::make_tuple(),
                                 std::forward_as_tuple(args...));
  }
};
}  // namespace ck

#endif