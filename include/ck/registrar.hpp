#ifndef CK_REGISTRAR_HPP
#define CK_REGISTRAR_HPP

#include <ck/pup.hpp>
#include <ck/singleton.hpp>
#include <ck/traits.hpp>

namespace ck {
using register_fn_t = void (*)(void);
using index_fn_t = int (*)(void);

CK_GENERATE_SINGLETON(std::vector<register_fn_t>, chare_registry);

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
  // TODO ( might need to move this to a singleton )
  static std::vector<index_fn_t> __entries;

  static void __register(void) {
    __idx = CkRegisterChare("???", sizeof(Base), TypeArray);
    // CkRegisterArrayDimensions(__idx, 1);
    CkRegisterBase(__idx, CkIndex_ArrayElement::__idx);

    for (auto& entry : __entries) {
      (*entry)();
    }
  }

  template <index_fn_t Fn>
  static int __append(void) {
    auto ep = (int)__entries.size();
    __entries.emplace_back(Fn);
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
int index<Base>::__idx;

template <typename Base>
std::vector<index_fn_t> index<Base>::__entries;

namespace {
template <typename Base, typename... Args>
std::tuple<Args...> __arguments(void (Base::*)(Args...));
}

template <auto Fn>
using method_arguments_t = decltype(__arguments(Fn));

template <class Base, auto Entry>
struct method_registrar {
  static int __idx;

  static void __call(void* msg, void* obj) {
    using arguments_t = method_arguments_t<Entry>;
    using tuple_t = decay_tuple_t<arguments_t>;
    unpacker<tuple_t> t(msg);
    std::apply([&](auto... args) { (((Base*)obj)->*Entry)(args...); },
               std::move(t.value()));
  }

  static int __register(void) {
    // force the compiler to initialize this variable
    (void)__idx;
    return CkRegisterEp("???", &method_registrar<Base, Entry>::__call,
                        CMessage_CkMessage::__idx, index<Base>::__idx, 0);
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
    (void)__idx;
    return CkRegisterEp("???", &constructor_registrar<Base, Args...>::__call,
                        CMessage_CkMessage::__idx, index<Base>::__idx, 0);
  }
};

template <class Base, typename... Args>
int constructor_registrar<Base, Args...>::__idx =
    index<Base>::template __append<
        &ck::index<Base>::template constructor_index<Args...>>();
}  // namespace ck

#endif
