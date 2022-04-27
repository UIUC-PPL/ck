#ifndef CK_OPTIONS_HPP
#define CK_OPTIONS_HPP

#include <ck/index.hpp>
#include <ck/traits.hpp>

namespace ck {

// options given to a constructor for a chare-array
template <typename Index>
struct array_options : public CkArrayOptions {
  using index_t = Index;
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

  array_options(const index_t& stop)
      : array_options(index_view<index_t>::encode(stop)) {}

  array_options(const array_index_t& stop)
      : CkArrayOptions(CkArrayIndex(__get_dimension(stop), 0), stop,
                       CkArrayIndex(__get_dimension(stop), 1)) {}

  array_options(const index_t& start, const index_t& stop)
      : array_options(index_view<index_t>::encode(start),
                      index_view<index_t>::encode(stop)) {}

  array_options(const array_index_t& start, const array_index_t& stop)
      : CkArrayOptions(start, stop,
                       CkArrayIndex(__get_dimension(start, stop), 1)) {}

  array_options(const index_t& start, const index_t& stop, const index_t& step)
      : array_options(index_view<index_t>::encode(start),
                      index_view<index_t>::encode(stop),
                      index_view<index_t>::encode(step)) {}

  array_options(const array_index_t& start, const array_index_t& stop,
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
