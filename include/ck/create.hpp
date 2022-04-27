#ifndef CK_CREATE_HPP
#define CK_CREATE_HPP

#include <ck/proxy.hpp>

namespace ck {

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
collection_proxy<Base, Kind> __create_grouplike(const CkEntryOptions *opts,
                                                Args &&...args) {
  constexpr auto is_group = std::is_same_v<Kind, group>;
  constexpr auto msg_type = is_group ? BocInitMsg : NodeBocInitMsg;
  auto idx = index<Base>::__idx;
  auto ctor = index<Base>::template constructor_index<std::decay_t<Args>...>();
  auto *msg =
      ck::pack(const_cast<CkEntryOptions *>(opts), std::forward<Args>(args)...);
  UsrToEnv(msg)->setMsgtype(msg_type);
  if constexpr (is_group) {
    return CkCreateGroup(idx, ctor, msg);
  } else {
    return CkCreateNodeGroup(idx, ctor, msg);
  }
}
}  // namespace

// creates a proxy with the given arguments
template <typename Proxy>
struct creator;

// array creator with options given
template <typename Base, typename Index>
struct creator<array_proxy<Base, Index>> {
  template <typename... Args>
  array_proxy<Base, Index> operator()(const constructor_options<Base> &opts,
                                      Args &&...args) const {
    return __create_array<Base, Index>(opts, std::forward<Args>(args)...);
  }
};

// group creator with no options given
template <typename Base>
struct creator<group_proxy<Base>> {
  template <typename... Args>
  group_proxy<Base> operator()(const CkEntryOptions *opts,
                               Args &&...args) const {
    return __create_grouplike<Base, group>(opts, std::forward<Args>(args)...);
  }
};

template <typename Base>
struct creator<nodegroup_proxy<Base>> {
  template <typename... Args>
  nodegroup_proxy<Base> operator()(const CkEntryOptions *opts,
                                   Args &&...args) const {
    return __create_grouplike<Base, nodegroup>(opts,
                                               std::forward<Args>(args)...);
  }
};

// creator for singleton chares
template <typename Base>
struct creator<chare_proxy<Base>> {
  template <typename... Args>
  chare_proxy<Base> operator()(const on_pe &opts, Args &&...args) const {
    CkChareID ret;
    auto *epopts = const_cast<CkEntryOptions *>(opts.opts);
    auto *msg = ck::pack(epopts, std::forward<Args>(args)...);
    auto ctor =
        index<Base>::template constructor_index<std::decay_t<Args>...>();
    CkCreateChare(index<Base>::__idx, ctor, msg, &ret, opts.which);
    return ret;
  }
};

template <typename T>
struct options_of {
  using type = CkEntryOptions *;
};

template <typename T, typename Index>
struct options_of<element_proxy<T, array<Index>>> {
  using type = on_pe;
};

template <typename T>
struct options_of<chare_proxy<T>> {
  using type = on_pe;
};

template <typename T, typename Index>
struct options_of<collection_proxy<T, array<Index>>> {
  using type = constructor_options<T>;
};

template <typename T>
using options_of_t = typename options_of<T>::type;

template <typename Proxy, std::size_t... Is, std::size_t... Js,
          typename... Args>
auto __create(std::index_sequence<Is...>, std::index_sequence<Js...>,
              std::tuple<Args...> args) {
  using local_t = typename Proxy::local_t;
  using options_t = options_of_t<Proxy>;
  constexpr auto empty = sizeof...(Args) == 0;
  if constexpr (empty && Proxy::is_array) {
    CkArrayOptions opts;
    return CProxy_ArrayBase::ckCreateEmptyArray(opts);
  } else {
    options_t opts(std::get<Js>(std::move(args))...);
    return creator<Proxy>()(opts, std::get<Is>(std::move(args))...);
  }
}

// TODO ( enable asynchronous (array?) creation )
template <typename Proxy, typename... Args>
auto create(Args &&...args) {
  if constexpr (is_section_v<Proxy>) {
    return Proxy(std::move(args)...);
  } else {
    using last_t = std::decay_t<get_last_t<Args...>>;
    using local_t = typename Proxy::local_t;
    using options_t = options_of_t<Proxy>;
    constexpr auto N = (std::is_same_v<options_t, std::remove_cv_t<last_t>>)
                           ? (sizeof...(args) - 1)
                           : longest_match_v<local_t, Args...>;
    auto is = std::make_index_sequence<N>();
    auto js = make_index_range<N, sizeof...(Args)>();
    return __create<Proxy>(is, js, std::forward_as_tuple(args...));
  }
}

// TODO ( enable passing PE as argument )
template <typename Kind, typename Index, typename... Args>
auto insert(const element_proxy<Kind, array<Index>> &elt, Args &&...args) {
  return elt.template insert<Args...>(std::forward<Args>(args)...);
}
}  // namespace ck

#endif
