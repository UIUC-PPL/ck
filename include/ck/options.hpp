#ifndef CK_OPTIONS_HPP
#define CK_OPTIONS_HPP

#include <ck/index.hpp>
#include <ck/traits.hpp>

namespace ck {

namespace {
template <std::size_t N>
std::array<int, N> __to_integer_array(const void* data) {
  std::array<int, N> res;
  if constexpr (N <= CK_ARRAYINDEX_MAXLEN) {
    auto* ints = reinterpret_cast<const int*>(data);
    std::copy(ints, ints + N, res.begin());
  } else {
    auto* shorts = reinterpret_cast<const short*>(data);
    std::transform(shorts, shorts + N, res, [](short s) { return (int)s; });
  }
  return res;
}
}  // namespace

template <typename T, typename Enable = void>
struct constructor_options;

template <typename T>
struct constructor_options<T, std::enable_if_t<is_array_v<kind_of_t<T>>>> {
  using index_t = index_of_t<kind_of_t<T>>;
  using array_index_t = array_index_of_t<index_t>;
  static constexpr auto N = dimensionality_of_v<index_t>;

  CkArrayOptions array;

  constructor_options(const index_t& stop)
      : array(N, __to_integer_array<N>(&stop).data()) {}

  constructor_options(const array_index_t& stop)
      : constructor_options(*reinterpret_cast<const index_t*>(stop.data())) {}

  constructor_options(const array_index_t& start, const array_index_t& stop,
                      const array_index_t& step)
      : array(start, stop, step) {}

  constructor_options(const index_t& start, const index_t& stop,
                      const index_t& step)
      : constructor_options(index_view<index_t>::encode(start),
                            index_view<index_t>::encode(stop),
                            index_view<index_t>::encode(step)) {}
};
}  // namespace ck

#endif
