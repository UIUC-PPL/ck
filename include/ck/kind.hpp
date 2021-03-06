#ifndef CK_KIND_HPP
#define CK_KIND_HPP

#include <charm++.h>

namespace ck {
using kind_t = ChareType;

struct group {
  using index_t = int;
  using parent_t = Group;
  using collection_proxy_t = CProxy_IrrGroup;
  using element_proxy_t = CProxyElement_IrrGroup;
  using section_proxy_t = CProxySection_IrrGroup;
  static constexpr kind_t kind = TypeGroup;
  static constexpr auto& __idx = CkIndex_Group::__idx;
};

struct nodegroup {
  using index_t = int;
  using parent_t = NodeGroup;
  using collection_proxy_t = CProxy_NodeGroup;
  using element_proxy_t = CProxyElement_NodeGroup;
  using section_proxy_t = CProxySection_NodeGroup;
  static constexpr kind_t kind = TypeNodeGroup;
  static constexpr auto& __idx = CkIndex_NodeGroup::__idx;
};

template <class Index>
struct array {
  using index_t = Index;
  using parent_t = ArrayElement;
  using collection_proxy_t = CProxy_ArrayElement;
  using element_proxy_t = CProxyElement_ArrayElement;
  using section_proxy_t = CProxySection_ArrayElement;
  static constexpr kind_t kind = TypeArray;
  static constexpr auto& __idx = CkIndex_ArrayElement::__idx;
};

struct singleton_chare {
  using parent_t = Chare;
  using proxy_t = CProxy_Chare;
  static constexpr kind_t kind = TypeChare;
  static constexpr auto& __idx = CkIndex_Chare::__idx;
};

struct main_chare : public singleton_chare {
  static constexpr kind_t kind = TypeMainChare;
};

template <typename T>
struct kind_of;

template <typename Base>
using kind_of_t = typename kind_of<Base>::type;

template <typename T>
struct base_index_of;

// Retrieve the base class's index of T for use with CkRegisterBase
template <typename T>
static constexpr auto& base_index_of_v = base_index_of<T>::value;

template <typename T>
using index_of_t = typename T::index_t;

template <typename T>
using parent_of_t = typename T::parent_t;

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

template <typename T>
constexpr auto is_main_chare_v = std::is_same_v<main_chare, kind_of_t<T>>;
}  // namespace ck

#endif
