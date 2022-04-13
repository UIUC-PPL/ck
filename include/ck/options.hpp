#ifndef CK_OPTIONS_HPP
#define CK_OPTIONS_HPP

#include <ck/index.hpp>
#include <ck/traits.hpp>

namespace ck {
struct method_options {
  CkEntryOptions entry;
};

// helper struct to initialize charm++ options
template <typename... Ts>
struct options_initializer;

template <>
struct options_initializer<CkArrayOptions, int, int> {
  void operator()(CkArrayOptions& opts, int stop) const {
    new (&opts) CkArrayOptions(stop);
  }
};

template <>
struct options_initializer<CkArrayOptions, int, int, int, int> {
  void operator()(CkArrayOptions& opts, int start, int stop, int step) const {
    new (&opts) CkArrayOptions(CkArrayIndex1D(start), CkArrayIndex1D(stop),
                               CkArrayIndex1D(step));
  }
};

template <typename T, typename Enable = void>
struct constructor_options;

template <typename T>
struct constructor_options<T, std::enable_if_t<is_array_chare_v<T>>>
    : public method_options {
  using index_t = array_index_of_t<T>;

 private:
  storage_t<CkArrayOptions> storage_;

 public:
  CkArrayOptions& array;

  template <typename... Args>
  constructor_options(Args&&... args) : array((CkArrayOptions&)storage_) {
    // delay initialization of the array options so we
    // can use the helper struct to initialize them
    options_initializer<CkArrayOptions, index_t, std::decay_t<Args>...>()(
        array, std::forward<Args>(args)...);
  }
};
}  // namespace ck

#endif
