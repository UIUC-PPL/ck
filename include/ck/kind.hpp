#ifndef CK_KIND_HPP
#define CK_KIND_HPP

#include <charm++.h>

namespace ck {
using kind_t = ChareType;

struct group {
  using index_t = int;
  using element_t = Group;
  using collection_proxy_t = CProxy_IrrGroup;
  using element_proxy_t = CProxyElement_IrrGroup;
  using section_proxy_t = CProxySection_IrrGroup;
  static constexpr kind_t kind = TypeGroup;
  static constexpr auto& __idx = CkIndex_Group::__idx;
};

struct nodegroup {
  using index_t = int;
  using element_t = NodeGroup;
  using collection_proxy_t = CProxy_NodeGroup;
  using element_proxy_t = CProxyElement_NodeGroup;
  using section_proxy_t = CProxySection_NodeGroup;
  static constexpr kind_t kind = TypeNodeGroup;
  static constexpr auto& __idx = CkIndex_NodeGroup::__idx;
};

template <class Index>
struct array {
  using index_t = Index;
  using element_t = ArrayElement;
  using collection_proxy_t = CProxy_ArrayElement;
  using element_proxy_t = CProxyElement_ArrayElement;
  using section_proxy_t = CProxySection_ArrayElement;
  static constexpr kind_t kind = TypeArray;
  static constexpr auto& __idx = CkIndex_ArrayElement::__idx;
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
using collection_proxy_of_t = typename T::collection_proxy_t;

template <typename T>
using element_proxy_of_t = typename T::element_proxy_t;

template <typename T>
using section_proxy_of_t = typename T::section_proxy_t;

template <typename T>
struct is_array : public std::false_type {};

template <typename Index>
struct is_array<array<Index>> : public std::true_type {};

template <typename T>
constexpr auto is_array_v = is_array<T>::value;
}  // namespace ck

#endif
