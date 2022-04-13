#ifndef CK_REGISTRAR_HPP
#define CK_REGISTRAR_HPP

#include <ck/index.hpp>
#include <ck/pup.hpp>
#include <ck/singleton.hpp>

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
    using array_index_t = array_index_of_t<Base>;
    constexpr auto ndims = index_view<array_index_t>::dimensionality;

    __idx = CkRegisterChare(__PRETTY_FUNCTION__, sizeof(Base), TypeArray);
    CkRegisterArrayDimensions(__idx, ndims);
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

template <typename Message>
int __message_idx(void) {
  return Message::__idx;
}
}  // namespace

template <typename... Args>
struct method_attributes {
  static constexpr auto is_message = is_message_v<std::decay_t<Args>...>;
  static constexpr auto flags = is_message ? 0 : CK_EP_NOKEEP;
  static int __idx;
};

template <typename... Args>
int method_attributes<Args...>::__idx = ([](void) {
  if constexpr (is_message) {
    return __message_idx<std::remove_pointer_t<Args>...>();
  } else {
    // wildcard -- accepts marshal, reduction messages, etc.
    return 0;
  }
})();

template <typename T>
struct attributes_of;

template <typename... Ts>
struct attributes_of<std::tuple<Ts...>> {
  using type = method_attributes<Ts...>;
};

template <auto Fn>
using method_arguments_t = decltype(__arguments(Fn));

template <auto Fn>
using attributes_of_t = typename attributes_of<method_arguments_t<Fn>>::type;

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
    using attributes_t = attributes_of_t<Entry>;
    return CkRegisterEp(
        __PRETTY_FUNCTION__, &method_registrar<Base, Entry>::__call,
        attributes_t::__idx, index<Base>::__idx, attributes_t::flags);
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
    using attributes_t = method_attributes<Args...>;
    return CkRegisterEp(
        __PRETTY_FUNCTION__, &constructor_registrar<Base, Args...>::__call,
        attributes_t::__idx, index<Base>::__idx, attributes_t::flags);
  }
};

template <class Base, typename... Args>
int constructor_registrar<Base, Args...>::__idx =
    index<Base>::template __append<
        &ck::index<Base>::template constructor_index<Args...>>();
}  // namespace ck

#endif
