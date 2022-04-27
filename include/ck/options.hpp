#ifndef CK_OPTIONS_HPP
#define CK_OPTIONS_HPP

#include <ck/index.hpp>
#include <ck/traits.hpp>

namespace ck {

// options given to a constructor for a given class
template <typename T, typename Enable = void>
struct constructor_options;

// options given to a constructor for a chare-array
template <typename T>
struct constructor_options<T, std::enable_if_t<is_array_v<kind_of_t<T>>>>
    : public CkArrayOptions {
  using index_t = index_of_t<kind_of_t<T>>;
  using array_index_t = array_index_of_t<index_t>;

 private:
  // returns and validates the dimensions of a set of indices
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
  CkEntryOptions* e_opts = nullptr;

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

struct on_pe {
  int which;
  const CkEntryOptions* opts;

  on_pe(const CkEntryOptions* opts_ = nullptr) : on_pe(CK_PE_ANY, opts_) {}

  on_pe(int which_, const CkEntryOptions* opts_ = nullptr)
      : which(which_), opts(opts_) {}
};
}  // namespace ck

#endif
