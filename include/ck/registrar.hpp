#ifndef CK_REGISTRAR_HPP
#define CK_REGISTRAR_HPP

#include <ck/attributes.hpp>
#include <ck/index.hpp>
#include <ck/kind.hpp>
#include <ck/pup.hpp>

namespace ck {
using index_fn_t = int (*)(void);

template <typename Base>
int main_chare_constructor(void);

template <class Base>
struct chare_registrar {
  static int __idx;
  static int __register(void);
};

template <class Base>
int chare_registrar<Base>::__idx = __register();

template <class Base, typename... Args>
struct constructor_registrar;

template <class Base, auto Entry>
struct method_registrar;

template <typename Base>
struct index {
  static int __idx;

  static std::vector<index_fn_t>& __entries(void) {
    static std::vector<index_fn_t> entries;
    return entries;
  }

  static void __register(void) {
    using collection_kind_t = kind_of_t<Base>;

    __idx = CkRegisterChare(__PRETTY_FUNCTION__, sizeof(Base),
                            collection_kind_t::kind);

    auto base = base_index_of_v<Base>;
    CkEnforceMsg(base >= 0, "derived class init'd before parent");
    CkRegisterBase(__idx, base);

    if constexpr (is_array_v<collection_kind_t>) {
      using collection_index_t = index_of_t<collection_kind_t>;
      constexpr auto ndims = index_view<collection_index_t>::dimensionality;
      CkRegisterArrayDimensions(__idx, ndims);
    } else if constexpr (std::is_same_v<main_chare, collection_kind_t>) {
      CkRegisterMainChare(__idx, main_chare_constructor<Base>());
    }

    CkRegisterMigCtor(__idx, constructor_index<CkMigrateMessage*>());

    for (auto& entry : __entries()) {
      (*entry)();
    }
  }

  template <index_fn_t Fn>
  static int __append(void) {
    auto& entries = __entries();
    auto ep = (int)entries.size();
    entries.emplace_back(Fn);
    return ep;
  }

  template <typename... Args>
  static int constructor_index(void) {
    static auto ep = constructor_registrar<Base, Args...>::__register();
    return ep;
  }

  template <auto Entry>
  static int method_index(void) {
    static auto ep = method_registrar<Base, Entry>::__register();
    return ep;
  }
};

template <typename Base>
int index<Base>::__idx = -1;

template <class Base, auto Entry>
struct method_registrar {
  static int __idx;

  static constexpr auto is_exclusive = is_exclusive_v<Entry>;
  static constexpr auto is_threaded = is_threaded_v<Entry>;

  static_assert(!is_exclusive ||
                (std::is_same_v<nodegroup, kind_of_t<Base>> && !is_threaded));

  static void __call(void* msg, void* obj) {
    using helper_t = member_getter<Entry>;
    if constexpr (helper_t::is_method) {
      using arguments_t = method_arguments_t<Entry>;
      using tuple_t = decay_tuple_t<arguments_t>;
      unpacker<tuple_t> t(msg);
      std::apply(
          [&](auto... args) { (((Base*)obj)->*Entry)(std::move(args)...); },
          std::move(t.value()));
    } else {
      using mailbox_t = typename helper_t::type;
      using tuple_t = std::tuple<typename mailbox_t::type>;
      unpacker<tuple_t> t(msg);
      (static_cast<Base*>(obj)->*Entry).post(std::move(std::get<0>(t.value())));
    }
  }

  static void __thrcall(void* arg_) {
    auto* arg = reinterpret_cast<CkThrCallArg*>(arg_);
    auto *msg = arg->msg, *obj = arg->obj;
    delete arg;
    __call(msg, obj);
  }

  static void __call_threaded(void* msg, void* obj) {
    // TODO ( allow users to specify the stack size here )
    auto tid = CthCreate(__thrcall, new CkThrCallArg(msg, obj), 0);
    (reinterpret_cast<Chare*>(obj))->CkAddThreadListeners(tid, msg);
    CthTraceResume(tid);
    CthResume(tid);
  }

  static void __call_exclusive(void* msg, void* obj) {
    auto* grp = reinterpret_cast<NodeGroup*>(obj);
    if (CmiTryLock(grp->__nodelock)) {
      auto ep = ck::index<Base>::template method_index<Entry>();
      CkSendMsgNodeBranch(ep, msg, CkMyNode(), grp->CkGetNodeGroupID());
      return;
    }
    __call(msg, obj);
    CmiUnlock(grp->__nodelock);
  }

  // choose which call function via user attributes
  static constexpr auto __choose_call(void) {
    if constexpr (is_threaded) {
      return __call_threaded;
    } else if constexpr (is_exclusive) {
      return __call_exclusive;
    } else {
      return __call;
    }
  }

  static int __register(void) {
    // force the compiler to initialize this variable
    __dummy(__idx);
    return CkRegisterEp(__PRETTY_FUNCTION__, __choose_call(),
                        message_index_v<Entry>, index<Base>::__idx,
                        registration_flags<Entry>);
  }
};

template <class Base, auto Entry>
int method_registrar<Base, Entry>::__idx = index<Base>::template __append<
    &ck::index<Base>::template method_index<Entry>>();

template <class Base, typename... Args>
struct constructor_registrar {
  static int __idx;

  static void __call(void* msg, void* obj) {
    using tuple_t = std::tuple<std::decay_t<Args>...>;
    unpacker<tuple_t> t(msg);
    std::apply(
        [&](Args&&... args) {
          new ((Base*)obj) Base(std::forward<Args>(args)...);
        },
        std::move(t.value()));
  }

  static int __register(void) {
    // force the compiler to initialize this variable
    __dummy(__idx);
    constexpr auto is_message = is_message_v<Args...>;
    constexpr auto flags = is_message ? 0 : CK_EP_NOKEEP;
    return CkRegisterEp(__PRETTY_FUNCTION__,
                        &constructor_registrar<Base, Args...>::__call,
                        message_index_of_v<Args...>, index<Base>::__idx, flags);
  }
};

template <class Base>
struct constructor_registrar<Base, CkMigrateMessage*> {
  static int __idx;

  static void __call(void* msg, void* obj) {
    call_migration_constructor<Base> caller(obj);
    caller(reinterpret_cast<CkMigrateMessage*>(msg));
  }

  static int __register(void) {
    // force the compiler to initialize this variable
    __dummy(__idx);
    return CkRegisterEp(__PRETTY_FUNCTION__,
                        &constructor_registrar<Base, CkMigrateMessage*>::__call,
                        0, index<Base>::__idx, 0);
  }
};

template <class Base, typename... Args>
int constructor_registrar<Base, Args...>::__idx =
    index<Base>::template __append<
        &ck::index<Base>::template constructor_index<Args...>>();

template <class Base>
int constructor_registrar<Base, CkMigrateMessage*>::__idx =
    index<Base>::template __append<
        &ck::index<Base>::template constructor_index<CkMigrateMessage*>>();
}  // namespace ck

#endif
