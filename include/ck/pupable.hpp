#ifndef CK_PUPABLE_HPP
#define CK_PUPABLE_HPP

#include <ck/common.hpp>

namespace ck {
using pup_id_t = PUP::able::PUP_ID;

template <typename T, typename Parent = PUP::able>
struct pupable : public Parent {
 private:
  static pup_id_t __id;
  static int __idx;

 protected:
  template <typename... Args>
  pupable(Args&&... args) : Parent(std::forward<Args>(args)...) {
    __dummy(__idx);
  }

 public:
  virtual const pup_id_t& get_PUP_ID(void) const override { return __id; }

 private:
  static PUP::able* __call(void) {
    auto* obj = ::operator new(sizeof(T));
    call_migration_constructor<T> caller(obj);
    caller((CkMigrateMessage*)nullptr);
    return reinterpret_cast<PUP::able*>(obj);
  }

  static void __register(void) {
    __id = PUP::able::register_constructor(typeid(T).name(), __call);
  }
};

template <typename T, typename Parent>
pup_id_t pupable<T, Parent>::__id;

template <typename T, typename Parent>
int pupable<T, Parent>::__idx = ([](void) {
  auto& reg = registry::pupables();
  auto idx = (int)reg.size();
  reg.emplace_back(&(pupable<T, Parent>::__register));
  return idx;
})();
}  // namespace ck

#endif
