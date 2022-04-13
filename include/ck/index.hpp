#ifndef CK_INDEX_HPP
#define CK_INDEX_HPP

#include <ckarrayindex.h>

namespace ck {
template <typename T>
struct array_index_of;

template <typename Base>
using array_index_of_t = typename array_index_of<Base>::type;

template <typename T, typename Enable = void>
struct index_view;

namespace {
template <typename T>
constexpr int __dimensionality(void) {
  if constexpr (std::is_same_v<T, CkArrayIndex1D>) {
    return 1;
  } else if constexpr (std::is_same_v<T, CkArrayIndex2D>) {
    return 2;
  } else if constexpr (std::is_same_v<T, CkArrayIndex3D>) {
    return 3;
  } else if constexpr (std::is_same_v<T, CkArrayIndex4D>) {
    return 4;
  } else if constexpr (std::is_same_v<T, CkArrayIndex5D>) {
    return 5;
  } else if constexpr (std::is_same_v<T, CkArrayIndex6D>) {
    return 6;
  } else {
    return 0;
  }
}
}  // namespace

template <typename T>
struct index_view<T, std::enable_if_t<std::is_base_of_v<CkArrayIndex, T>>> {
  constexpr static auto dimensionality = __dimensionality<T>();

  static const T& encode(const T& index) { return index; }

  static T& decode(CkArrayIndex& index) { return (T&)index; }

  static const T& decode(const CkArrayIndex& index) { return (const T&)index; }
};

template <>
struct index_view<int> {
  constexpr static auto dimensionality = 1;

  static CkArrayIndex encode(int index) { return CkArrayIndex1D(index); }

  template <typename T>
  static auto& decode(T& index) {
    return (index.data())[0];
  }
};
}  // namespace ck

#endif
