#ifndef CK_KIND_HPP
#define CK_KIND_HPP

#include <charm++.h>

namespace ck {
using kind_t = ChareType;

struct group {
  using index_t = int;
  using element_t = Group;
  using proxy_t = CProxy_IrrGroup;
  using element_proxy_t = CProxyElement_IrrGroup;
  using section_proxy_t = CProxySection_IrrGroup;
};

struct nodegroup {
  using index_t = int;
  using element_t = NodeGroup;
  using collection_proxy_t = CProxy_NodeGroup;
  using element_proxy_t = CProxyElement_NodeGroup;
  using section_proxy_t = CProxySection_NodeGroup;
};

template <class Index>
struct array {
  using index_t = Index;
  using element_t = ArrayElement;
  using collection_proxy_t = CProxy_ArrayElement;
  using element_proxy_t = CProxyElement_ArrayElement;
  using section_proxy_t = CProxySection_ArrayElement;
};

template <typename T>
struct kind_of;

template <typename Base>
using kind_of_t = typename kind_of<Base>::type;

template <typename T>
using index_of_t = typename T::index_t;

template <typename T>
using element_of_t = typename T::element_t;

template <typename T>
using element_proxy_of_t = typename T::element_proxy_t;
}  // namespace ck

#endif
