#ifndef CK_CHARE_HPP
#define CK_CHARE_HPP

#include <ck/proxy.hpp>

namespace ck {

template <class Base, class Kind>
struct chare : public element_of_t<Kind>, public CBase {
  using parent_t = element_of_t<Kind>;
  using collection_index_t = index_of_t<Kind>;
  using CProxy_Derived = array_proxy<Base, Kind>;

  CBASE_MEMBERS;

  // duplicated for node/groups but c'est la vie
  collection_index_t thisIndex;

  template <typename... Args>
  chare(Args &&...args)
      : parent_t(std::forward<Args>(args)...),
        thisProxy(static_cast<parent_t *>(this)) {
    // force the compiler to initialize this variable
    (void)chare_registrar<Base>::__idx;
    // conditionally initialize this index
    if constexpr (std::is_same_v<ArrayElement, parent_t>) {
      thisIndex =
          index_view<collection_index_t>::decode(parent_t::thisIndexMax);
    } else {
      thisIndex = parent_t::thisIndex;
    }
  }

  void parent_pup(PUP::er &p) {
    recursive_pup<Base>(static_cast<Base *>(this), p);
  }
};

template <typename T>
struct kind_of {
  template <class Base, class Kind>
  static Kind kind_(chare<Base, Kind> &);

  using type = decltype(kind_(std::declval<T &>()));
};

template <class Base, class Kind>
void chare<Base, Kind>::virtual_pup(PUP::er &p) {
  recursive_pup<Base>(static_cast<Base *>(this), p);
}
}  // namespace ck

#endif
