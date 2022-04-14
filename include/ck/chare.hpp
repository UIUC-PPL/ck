#ifndef CK_CHARE_HPP
#define CK_CHARE_HPP

#include <ck/proxy.hpp>

namespace ck {
template <class Base, class Index>
struct chare : public ArrayElement, public CBase {
  using array_index_t = Index;
  using CProxy_Derived = array_proxy<Base, array_index_t>;

  CBASE_MEMBERS;

  array_index_t thisIndex;

  template <typename... Args>
  chare(Args &&...args)
      : ArrayElement(std::forward<Args>(args)...),
        thisProxy(static_cast<ArrayElement *>(this)),
        thisIndex(index_view<Index>::decode(thisIndexMax)) {
    // force the compiler to initialize this variable
    (void)chare_registrar<Base>::__idx;
  }

  void parent_pup(PUP::er &p) {
    recursive_pup<Base>(static_cast<Base *>(this), p);
  }
};

template <typename T>
struct array_index_of {
  template <class Base, class Index>
  static Index idx__(chare<Base, Index> &);

  using type = decltype(idx__(std::declval<T &>()));
};

template <class Base, class Index>
void chare<Base, Index>::virtual_pup(PUP::er &p) {
  recursive_pup<Base>(static_cast<Base *>(this), p);
}
}  // namespace ck

#endif
