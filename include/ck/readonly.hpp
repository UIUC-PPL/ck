#ifndef CK_READONLY_HPP
#define CK_READONLY_HPP

#include <ck/traits.hpp>

// generate a readonly impl-name for a variable
#define CK_READONLY_NAME(name) __##name##__readonly

// generate a readonly impl-class for a variable
// along with its global (reference) declaration
#define CK_READONLY(type, name)                   \
  struct CK_READONLY_NAME(name) {                 \
    using T = CK_PP_UNPAREN(type);                \
    PUP::detail::TemporaryObjectHolder<T> value;  \
    void pup(PUP::er& p) { p | (this->value).t; } \
  };                                              \
  CK_PP_UNPAREN(type)& name =                     \
      (ck::readonly<CK_READONLY_NAME(name)>::__access()).value.t;

// forward declares a readonly variable
#define CK_EXTERN_READONLY(type, name) extern CK_PP_UNPAREN(type) & name;

namespace ck {

// helper class to pup, register, and store a readonly variable
template <typename T>
struct readonly {
  static int __idx;

  // accesses this readonly variable's stored value
  static T& __access(void) {
    // force the compiler to initialize this variable
    __dummy(readonly<T>::__idx);
    static PUP::detail::TemporaryObjectHolder<T> val;
    return val.t;
  }

  // pups this readonly variable
  static void __pup(void* p_) {
    auto& p = *((PUP::er*)p_);
    p | __access();
  }

  // registers this readonly variable with the RTS
  static void __register(void) {
    auto* name = typeid(T).name();
#if CMK_VERBOSE
    CkPrintf("%d> registering readonly: %s\n", CkMyPe(), name);
#endif
    CkRegisterReadonly(name, name, sizeof(T), (void*)&(__access()), &__pup);
  }
};

// adds a readonly to the list of readonlies registered at startup
template <typename T>
int readonly<T>::__idx = ([](void) {
  auto& reg = registry::readonlies();
  auto idx = (int)reg.size();
  reg.emplace_back(&(readonly<T>::__register));
  return idx;
})();
}  // namespace ck

#endif
