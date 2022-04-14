#ifndef CK_INDEX_HPP
#define CK_INDEX_HPP

#include <ck/traits.hpp>

namespace ck {
template <typename T>
struct array_index_of;

template <typename Base>
using array_index_of_t = typename array_index_of<Base>::type;

template <typename T, typename Enable = void>
struct index_view;

template <typename T>
constexpr bool is_builtin_index_v =
    std::is_same_v<T, CkIndex1D> || std::is_same_v<T, CkIndex2D> ||
    std::is_same_v<T, CkIndex3D> || std::is_same_v<T, CkIndex4D> ||
    std::is_same_v<T, CkIndex5D> || std::is_same_v<T, CkIndex6D> ||
    std::is_same_v<T, CkIndexMax>;

namespace {
template <typename T>
constexpr int __dimensionality(void) {
  if constexpr (std::is_same_v<T, CkArrayIndex1D> ||
                std::is_same_v<T, CkIndex1D>) {
    return 1;
  } else if constexpr (std::is_same_v<T, CkArrayIndex2D> ||
                       std::is_same_v<T, CkIndex2D>) {
    return 2;
  } else if constexpr (std::is_same_v<T, CkArrayIndex3D> ||
                       std::is_same_v<T, CkIndex3D>) {
    return 3;
  } else if constexpr (std::is_same_v<T, CkArrayIndex4D> ||
                       std::is_same_v<T, CkIndex4D>) {
    return 4;
  } else if constexpr (std::is_same_v<T, CkArrayIndex5D> ||
                       std::is_same_v<T, CkIndex5D>) {
    return 5;
  } else if constexpr (std::is_same_v<T, CkArrayIndex6D> ||
                       std::is_same_v<T, CkIndex6D>) {
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

  static const T& decode(const CkArrayIndex& index) { return (const T&)index; }
};

template <typename T>
struct index_view<T, std::enable_if_t<is_builtin_index_v<T>>> {
  constexpr static auto dimensionality = __dimensionality<T>();

  // TODO ( implement type-mapping from T -> CkArrayIndex(_)D )
  static CkArrayIndex encode(const T& index) {
    if constexpr (std::is_same_v<T, CkIndex1D>) {
      return CkArrayIndex1D(index);
    } else {
      static_assert(always_false<T>, "not implemented");
    }
  }

  static const T& decode(const CkArrayIndex& index) {
    return *((const T*)index.data());
  }
};
}  // namespace ck

#endif
