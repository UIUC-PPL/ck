#ifndef CK_SEND_HPP
#define CK_SEND_HPP

#include <ck/proxy.hpp>

namespace ck {
namespace {
// traces the creation of a message
inline void __trace_creation(envelope* env, int ep, CkEnvelopeType type,
                             std::size_t size) {
  env->setTotalsize(size);
  env->setMsgtype(type);
  _TRACE_CREATION_DETAILED(env, ep);
  _TRACE_CREATION_DONE(1);
}

// traces the start of an array element's execution
inline void __trace_begin_array_execute(ArrayElement* elt, int ep,
                                        std::size_t size) {
  envelope env;
  __trace_creation(&env, ep, ForArrayEltMsg, size);
  CmiObjId projID = (elt->ckGetArrayIndex()).getProjectionID();
  _TRACE_BEGIN_EXECUTE_DETAILED(CpvAccess(curPeEvent), ForArrayEltMsg, ep,
                                CkMyPe(), 0, &projID, elt);
#if CMK_LBDB_ON
  auto id = elt->ckGetID().getElementID();
  auto& aid = elt->ckGetArrayID();
  (aid.ckLocalBranch())->recordSend(id, size, CkMyPe());
#endif
}

// traces the start of an object's execution
template <typename Kind>
void __trace_begin_execute(Chare* obj, int ep, std::size_t size) {
  if constexpr (is_array_v<Kind>) {
    __trace_begin_array_execute(static_cast<ArrayElement*>(obj), ep, size);
  } else {
    envelope env;
    __trace_creation(&env, ep, message_type<Kind>(), size);
    _TRACE_BEGIN_EXECUTE(&env, obj);
  }
}

template <auto Entry, typename Attributes, typename Proxy, typename... Args>
auto __send(const Proxy& proxy, const CkEntryOptions* opts,
            const Args&... args) {
  using kind_t = typename Proxy::kind_t;
  using local_t = typename Proxy::local_t;
  constexpr auto is_array = Proxy::is_array;
  constexpr auto is_local = is_local_v<Entry> || contains_local_v<Attributes>;
  constexpr auto is_inline =
      is_inline_v<Entry> || contains_inline_v<Attributes>;
  // enforce that we can pass these arguments to the underlying EP
  static_assert(is_compatible_v<Entry, Args...>,
                "arguments incompatible with entry method");
  // we can try to look up element-like proxies
  if constexpr (is_elementlike_v<Proxy> && (is_local || is_inline)) {
    auto* local = proxy.ckLocal();
    // if the look up fails...
    if (local == nullptr) {
      // abort for [local] entry methods
      if constexpr (is_local) {
        CkAbort("local chare unavailable");
      }
    } else {
      // otherwise, determine the result of invoking the EP
      using result_t =
          decltype((local->*Entry)(std::forward<const Args&>(args)...));
      // trace inline entry methods. note that we cannot trace local
      // entry methods since their arguments are not always sizable.
      if constexpr (is_inline) {
        std::size_t size;
        // determine the size of the arguments
        if constexpr (is_message_v<Args...>) {
          size = UsrToEnv(args...)->getTotalsize();
        } else {
          auto pack = std::forward_as_tuple(const_cast<Args&>(args)...);
          size = sizeof(CkMarshallMsg) + PUP::size(pack);
        }
        // start tracing the object
        auto ep = get_entry_index<local_t, Entry>();
        __trace_begin_execute<kind_t>(local, ep, size);
      }
      // [local] EPs can return non-void values
      if constexpr (is_inline || std::is_same_v<void, result_t>) {
        CkCallstackPush(static_cast<Chare*>(local));
        (local->*Entry)(std::forward<const Args&>(args)...);
        CkCallstackPop(static_cast<Chare*>(local));
        _TRACE_END_EXECUTE();
        return;
      } else {
        CkCallstackPush(static_cast<Chare*>(local));
        auto res = (local->*Entry)(std::forward<const Args&>(args)...);
        CkCallstackPop(static_cast<Chare*>(local));
        _TRACE_END_EXECUTE();
        return res;
      }
    }
  }
  // local EPs will never reach this point anyway
  if constexpr (!is_local) {
    auto* msg = ck::pack(const_cast<CkEntryOptions*>(opts),
                         std::forward<const Args&>(args)...);
    auto ep = get_entry_index<local_t, Entry>();

    if constexpr (is_array) {
      UsrToEnv(msg)->setMsgtype(ForArrayEltMsg);
      // TODO ( add support for array createhere/home messages )
      ((CkArrayMessage*)msg)->array_setIfNotThere(CkArray_IfNotThere_buffer);
    }

    proxy.send(msg, ep, message_flags_v<Entry>);
  }
}

template <auto Entry, typename Attributes, typename Proxy, std::size_t... I0s,
          std::size_t... I1s, typename... Args>
auto __send(const Proxy& proxy,
            std::index_sequence<I0s...>,    // first args
            std::index_sequence<I1s...>,    // last args
            std::tuple<const Args&...> args /* all args */
) {
  return __send<Entry, Attributes>(proxy, std::get<I0s>(std::move(args))...,
                                   std::get<I1s>(std::move(args))...);
}

template <typename T>
constexpr auto is_entry_options_v =
    std::is_same_v<CkEntryOptions*, std::remove_cv_t<std::decay_t<T>>>;
}  // namespace

template <auto Entry, typename Attributes, typename Proxy, typename... Args>
auto send(const Proxy& proxy, const Args&... args) {
  // check if the last argument is an entry options pointer
  if constexpr (is_entry_options_v<get_last_t<Args...>>) {
    return __send<Entry, Attributes>(
        proxy, std::index_sequence<sizeof...(args) - 1>{},  // put last first
        std::make_index_sequence<sizeof...(args) - 1>{},    // put first last
        std::forward_as_tuple(args...));                    // bundled args
  } else {
    return __send<Entry, Attributes>(proxy, nullptr, args...);
  }
}

template <auto Entry, typename Proxy, typename... Args>
auto send(const Proxy& proxy, const Args&... args) {
  return send<Entry, ck::attributes<>>(proxy, args...);
}
}  // namespace ck

#endif
