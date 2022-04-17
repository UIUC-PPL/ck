#ifndef CK_READONLY_HPP
#define CK_READONLY_HPP

#include <ck/traits.hpp>

#define CK_READONLY_NAME(name) __##name##__readonly

#define CK_READONLY_IMPL(name) ck::readonly<CK_READONLY_NAME(name)>

#define CK_READONLY(type, name)               \
  struct CK_READONLY_NAME(name) {             \
    type value;                               \
    void pup(PUP::er& p) { p | this->value; } \
  };                                          \
  type& name = (CK_READONLY_IMPL(name)::__access()).value;

#define CK_EXTERN_READONLY(type, name) extern type& name;

namespace ck {

template <typename T>
struct readonly {
  static int __idx;
  static storage_t<T> __storage;

  static T& __access(void) {
    // force the compiler to initialize this variable
    __dummy(readonly<T>::__idx);
    // initialize readonly during first access
    static bool initialized = false;
    auto& t = reinterpret_cast<T&>(__storage);
    if (!initialized) {
      if constexpr (std::is_constructible_v<T, PUP::reconstruct>) {
        new (&t) T(PUP::reconstruct());
      } else {
        new (&t) T;
      }
      initialized = true;
    }
    return t;
  }

  static void __pup(void* p_) {
    auto& p = *((PUP::er*)p_);
    p | __access();
  }

  static void __register(void) {
    auto* name = typeid(T).name();
#if CMK_VERBOSE
    CkPrintf("%d> registering readonly: %s\n", CkMyPe(), name);
#endif
    CkRegisterReadonly(name, name, sizeof(T), (void*)&(__access()), &__pup);
  }
};

template <typename T>
int readonly<T>::__idx = ([](void) {
  auto& reg = registry::readonlies();
  auto idx = (int)reg.size();
  reg.emplace_back(&(readonly<T>::__register));
  return idx;
})();

template <typename T>
storage_t<T> readonly<T>::__storage;
}  // namespace ck

#endif
