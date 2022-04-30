#ifndef CK_CHARE_HPP
#define CK_CHARE_HPP

#include <ck/proxy.hpp>

namespace ck {

template <class Base, class Kind, class Parent = parent_of_t<Kind>>
struct chare : public Parent, public CBase {
  using parent_t = Parent;
  using collection_index_t = index_of_t<Kind>;
  using CProxy_Derived = collection_proxy<Base, Kind>;

  CBASE_MEMBERS;

  // duplicated for node/groups but c'est la vie
  collection_index_t thisIndex;

 protected:
  template <typename... Args>
  explicit chare(Args&&... args)
      : parent_t(std::forward<Args>(args)...),
        thisProxy(static_cast<parent_t*>(this)) {
    // force the compiler to initialize this variable
    __dummy(chare_registrar<Base>::__idx);
    // conditionally initialize this index
    if constexpr (std::is_same_v<ArrayElement, parent_t>) {
      thisIndex =
          index_view<collection_index_t>::decode(parent_t::thisIndexMax);
    } else {
      thisIndex = parent_t::thisIndex;
    }
  }

 public:
  void parent_pup(PUP::er& p) {
    recursive_pup<parent_t>(static_cast<parent_t*>(this), p);
  }
};

template <class Base, class Kind>
struct chare<Base, Kind, Chare> : public Chare, public CBase {
  using parent_t = Chare;
  using CProxy_Derived = chare_proxy<Base, Kind>;

  CBASE_MEMBERS;

  template <typename... Args>
  explicit chare(Args&&... args)
      : Chare(std::forward<Args>(args)...), thisProxy(this) {
    // force the compiler to initialize this variable
    __dummy(chare_registrar<Base>::__idx);
  }

  void parent_pup(PUP::er& p) {
    recursive_pup<parent_t>(static_cast<parent_t*>(this), p);
  }
};

template <class Base, class Kind, class Parent>
void chare<Base, Kind, Parent>::virtual_pup(PUP::er& p) {
  recursive_pup<Base>(static_cast<Base*>(this), p);
}

template <class Base, class Kind>
void chare<Base, Kind, Chare>::virtual_pup(PUP::er& p) {
  recursive_pup<Base>(static_cast<Base*>(this), p);
}

template <typename T>
struct kind_of {
  template <class Base, class Kind, class Parent>
  static Kind kind_(chare<Base, Kind, Parent>&);

  using type = decltype(kind_(std::declval<T&>()));
};
}  // namespace ck

#endif
