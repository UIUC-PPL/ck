#ifndef CK_EXTENDS_HPP
#define CK_EXTENDS_HPP

#include <ck/chare.hpp>

namespace ck {

// helper class to soundly extend a parent chare-type
template <typename Base, typename Parent>
struct extends : public Parent {
  using CProxy_Derived = instance_proxy_of_t<Base, kind_of_t<Parent>>;

  CBASE_MEMBERS;

  template <typename... Args>
  extends(Args&&... args)
      : Parent(std::forward<Args>(args)...), thisProxy(this) {
    // try to ensure parent comes before base
    // TODO ( need to enforce this more soundly )
    __dummy(chare_registrar<Parent>::__idx);
    __dummy(chare_registrar<Base>::__idx);
  }

  void parent_pup(PUP::er& p) {
    recursive_pup<Parent>(static_cast<Parent*>(this), p);
  }
};

template <typename Base, typename Parent>
void extends<Base, Parent>::virtual_pup(PUP::er& p) {
  recursive_pup<Base>(static_cast<Base*>(this), p);
}

template <typename T>
struct extends_of {
  static void __test(const void*);

  template <typename Base, typename Parent>
  static Parent __test(const extends<Base, Parent>*);

  using type = decltype(__test(std::declval<const T*>()));
};

template <typename T>
using extends_of_t = typename extends_of<T>::type;

template <typename T>
struct base_index_of {
 private:
  static constexpr auto& __value(void) {
    using extends_t = extends_of_t<T>;

    if constexpr (!std::is_same_v<void, extends_t>) {
      return ck::index<extends_t>::__idx;
    } else {
      return kind_of_t<T>::__idx;
    }
  }

 public:
  static constexpr auto& value = __value();
};
}  // namespace ck

#endif
