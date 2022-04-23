#ifndef CK_PUPABLE_HPP
#define CK_PUPABLE_HPP

#include <ck/common.hpp>

namespace ck {

// shorthand alias for pupable's id
using pup_id_t = PUP::able::PUP_ID;

// base class for a registered pupable
template <typename T, typename Parent = PUP::able>
struct pupable : public Parent {
 private:
  static pup_id_t __id;
  static int __idx;

 protected:
  template <typename... Args>
  pupable(Args&&... args) : Parent(std::forward<Args>(args)...) {
    // force the compiler to initialize this variable
    __dummy(__idx);
  }

 public:
  virtual const pup_id_t& get_PUP_ID(void) const override { return __id; }

 private:
  // allocates and initializes a pupable instance
  static PUP::able* __call(void) {
    auto* obj = ::operator new(sizeof(T));
    call_migration_constructor<T> caller(obj);
    caller((CkMigrateMessage*)nullptr);
    return reinterpret_cast<T*>(obj);
  }

  // register a pupable with charm++, setting its id
  static void __register(void) {
    __id = PUP::able::register_constructor(typeid(T).name(), __call);
  }
};

template <typename T, typename Parent>
pup_id_t pupable<T, Parent>::__id;

// append an pupable to the list of pupables to be registered
template <typename T, typename Parent>
int pupable<T, Parent>::__idx = ([](void) {
  auto& reg = registry::pupables();
  auto idx = (int)reg.size();
  reg.emplace_back(&(pupable<T, Parent>::__register));
  return idx;
})();
}  // namespace ck

#endif
