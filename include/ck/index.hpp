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
