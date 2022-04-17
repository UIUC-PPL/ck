#ifndef CK_INDEX_HPP
#define CK_INDEX_HPP

#include <ck/traits.hpp>

namespace ck {

template <typename T>
struct array_index_of {
  using type = CkArrayIndex;
};

template <>
struct array_index_of<CkIndex1D> {
  using type = CkArrayIndex1D;
};

template <>
struct array_index_of<CkIndex2D> {
  using type = CkArrayIndex2D;
};

template <>
struct array_index_of<CkIndex3D> {
  using type = CkArrayIndex3D;
};

template <>
struct array_index_of<CkIndex4D> {
  using type = CkArrayIndex4D;
};

template <>
struct array_index_of<CkIndex5D> {
  using type = CkArrayIndex5D;
};

template <>
struct array_index_of<CkIndex6D> {
  using type = CkArrayIndex6D;
};

template <>
struct array_index_of<CkIndexMax> {
  using type = CkArrayIndexMax;
};

template <typename T>
using array_index_of_t = typename array_index_of<T>::type;

template <typename T, typename Enable = void>
struct index_view;

template <typename T>
constexpr bool is_builtin_index_v =
    !std::is_same_v<CkArrayIndex, array_index_of_t<T>>;

template <typename T, typename Enable = void>
struct dimensionality_of {
 private:
  static constexpr int __value(void) {
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
      return -1;
    }
  }

 public:
  static constexpr auto value = __value();
};

template <typename T>
constexpr auto dimensionality_of_v = dimensionality_of<T>::value;

template <typename T>
struct index_view<T, std::enable_if_t<std::is_base_of_v<CkArrayIndex, T>>> {
  constexpr static auto dimensionality = dimensionality_of_v<T>;

  static const T& encode(const T& index) { return index; }

  static const T& decode(const CkArrayIndex& index) { return (const T&)index; }
};

template <typename T>
struct index_view<T, std::enable_if_t<is_builtin_index_v<T>>> {
  using array_index_t = array_index_of_t<T>;

  constexpr static auto dimensionality = dimensionality_of_v<T>;

  static array_index_t encode(const T& index) { return array_index_t(index); }

  static const T& decode(const CkArrayIndex& index) {
    return *((const T*)index.data());
  }
};
}  // namespace ck

#endif
