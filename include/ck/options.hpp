#ifndef CK_OPTIONS_HPP
#define CK_OPTIONS_HPP

#include <ck/index.hpp>
#include <ck/traits.hpp>

namespace ck {

template <typename Options, typename T, typename Enable = void>
struct options_initializer;

template <typename Index>
struct options_initializer<CkArrayOptions, std::tuple<Index>,
                           std::enable_if_t<is_builtin_index_v<Index>>> {
  void operator()(CkArrayOptions& opts, const Index& stop) const {
    if constexpr (std::is_same_v<CkIndex1D, Index>) {
      new (&opts) CkArrayOptions(stop);
    } else if constexpr (std::is_same_v<CkIndex2D, Index>) {
      new (&opts) CkArrayOptions(stop.x, stop.y);
    } else if constexpr (std::is_same_v<CkIndex3D, Index>) {
      new (&opts) CkArrayOptions(stop.x, stop.y, stop.z);
    } else if constexpr (std::is_same_v<CkIndex4D, Index>) {
      new (&opts) CkArrayOptions(stop.w, stop.x, stop.y, stop.z);
    } else if constexpr (std::is_same_v<CkIndex4D, Index>) {
      new (&opts) CkArrayOptions(stop.w, stop.x, stop.y, stop.z);
    } else if constexpr (std::is_same_v<CkIndex5D, Index>) {
      new (&opts) CkArrayOptions(stop.v, stop.w, stop.x, stop.y, stop.z);
    } else if constexpr (std::is_same_v<CkIndex6D, Index>) {
      new (&opts)
          CkArrayOptions(stop.x1, stop.y1, stop.z1, stop.x2, stop.y2, stop.z2);
    } else {
      static_assert(always_false<Index>, "not implemented");
    }
  }
};

template <typename T, typename Enable = void>
struct constructor_options;

template <typename T>
struct constructor_options<T, std::enable_if_t<is_array_v<kind_of_t<T>>>> {
  using index_t = index_of_t<kind_of_t<T>>;
  using array_index_t = array_index_of_t<index_t>;

 private:
  storage_t<CkArrayOptions> storage_;

 public:
  CkArrayOptions& array;

  constructor_options(void)
      : array(reinterpret_cast<CkArrayOptions&>(this->storage_)) {}

  constructor_options(const array_index_t& stop)
      : constructor_options(*reinterpret_cast<const index_t*>(stop.data())) {}

  constructor_options(const index_t& stop) : constructor_options() {
    options_initializer<CkArrayOptions, std::tuple<index_t>>()(this->array,
                                                               stop);
  }

  constructor_options(const index_t& start, const index_t& stop,
                      const index_t& step)
      : constructor_options(index_view<index_t>::encode(start),
                            index_view<index_t>::encode(stop),
                            index_view<index_t>::encode(step)) {}

  constructor_options(const constructor_options<T>& opts)
      : constructor_options() {
    new (&(this->array)) CkArrayOptions(opts.array);
  }

  constructor_options(const array_index_t& start, const array_index_t& stop,
                      const array_index_t& step)
      : constructor_options() {
    new (&(this->array)) CkArrayOptions(start, stop, step);
  }
};
}  // namespace ck

#endif
