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

template <typename Index>
struct options_initializer<CkArrayOptions, Index, Index> {
  void operator()(CkArrayOptions& opts, const Index& stop) const {
    if constexpr (std::is_same_v<Index, int>) {
      new (&opts) CkArrayOptions(stop);
    } else {
      // TODO ( implement other code-paths... )
      static_assert(always_false<Index>, "not implemented");
    }
  }
};

template <typename Index>
struct options_initializer<CkArrayOptions, Index, Index, Index, Index> {
  void operator()(CkArrayOptions& opts, const Index& start, const Index& stop,
                  const Index& step) const {
    new (&opts) CkArrayOptions(index_view<Index>::encode(start),
                               index_view<Index>::encode(stop),
                               index_view<Index>::encode(step));
  }
};

template <typename T, typename Enable = void>
struct constructor_options;

template <typename T>
struct constructor_options<T, std::enable_if_t<is_array_chare_v<T>>>
    : public method_options {
  using index_t = index_of_t<kind_of_t<T>>;

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
