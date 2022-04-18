#ifndef CK_OPTIONS_HPP
#define CK_OPTIONS_HPP

#include <ck/index.hpp>
#include <ck/traits.hpp>

namespace ck {

template <typename T, typename Enable = void>
struct constructor_options;

template <typename T>
struct constructor_options<T, std::enable_if_t<is_array_v<kind_of_t<T>>>>
    : public CkArrayOptions {
  using index_t = index_of_t<kind_of_t<T>>;
  using array_index_t = array_index_of_t<index_t>;

 private:
  template <typename... Ts>
  static auto __get_dimension(Ts&&... ts) {
    short result;
    auto fn = [](const auto& idx) { return idx.getDimension(); };
    auto pred = [&](const auto& idx) { return (result == fn(idx)); };
    // get the dimension of the first index
    ((result = fn(ts), (void)true) || ...);
    // check that all indices have the same dimension
    if constexpr (sizeof...(ts) >= 1) {
      auto count = (std::size_t(0) + ... + (pred(ts) ? 1 : 0));
      CkEnforceMsg((count == sizeof...(ts)), "dimensionality mismatch");
    }
    return result;
  }

 public:
  constructor_options(const index_t& stop)
      : constructor_options(index_view<index_t>::encode(stop)) {}

  constructor_options(const array_index_t& stop)
      : CkArrayOptions(CkArrayIndex(__get_dimension(stop), 0), stop,
                       CkArrayIndex(__get_dimension(stop), 1)) {}

  constructor_options(const index_t& start, const index_t& stop)
      : constructor_options(index_view<index_t>::encode(start),
                            index_view<index_t>::encode(stop)) {}

  constructor_options(const array_index_t& start, const array_index_t& stop)
      : CkArrayOptions(start, stop,
                       CkArrayIndex(__get_dimension(start, stop), 1)) {}

  constructor_options(const index_t& start, const index_t& stop,
                      const index_t& step)
      : constructor_options(index_view<index_t>::encode(start),
                            index_view<index_t>::encode(stop),
                            index_view<index_t>::encode(step)) {}

  constructor_options(const array_index_t& start, const array_index_t& stop,
                      const array_index_t& step)
      : CkArrayOptions(start, stop, step) {}
};
}  // namespace ck

#endif
