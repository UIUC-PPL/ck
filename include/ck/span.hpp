#ifndef CK_SPAN_HPP
#define CK_SPAN_HPP

#include <charm++.h>

namespace ck {

// a fixed-size container that can share ownership of a message buffer
template <typename T, std::size_t N>
struct nd_span {
  using source_type = std::shared_ptr<T>;
  using shape_type =
      std::conditional_t<(N == 1), std::size_t, std::array<std::size_t, N>>;
  using value_type = std::conditional_t<(N == 1), T&, nd_span<T, (N - 1)>>;

  static_assert(N >= 1);

  static constexpr auto dimensionality = N;

  template <typename, std::size_t>
  friend struct nd_span;

 private:
  shape_type shape_;  // comes first so its init'd first
  source_type source_;
  std::size_t offset_;

  // initialize each field of the span
  nd_span(const source_type& source, const shape_type& shape,
          std::size_t offset)
      : shape_(shape), source_(source), offset_(offset) {}

  // allocate a range of values, initializing each with the given arguments
  template <typename... Ts>
  static T* __alloc(std::size_t size, const Ts&... ts) {
    auto* t = (T*)(::operator new(size * sizeof(T)));
    if constexpr (!std::is_trivial_v<T>) {
      auto* end = t + size;
      for (auto it = t; it != end; it++) {
        new (it) T(std::forward<const Ts&>(ts)...);
      }
    }
    return t;
  }

 public:
  explicit nd_span(void) : offset_(0) {
    if constexpr (N == 1) {
      this->shape_ = 0;
    } else {
      std::fill(this->shape_.begin(), this->shape_.end(), 0);
    }
  }

  explicit nd_span(const source_type& source, const shape_type& shape)
      : nd_span(source, shape, 0) {}

  explicit nd_span(const shape_type& shape)
      : shape_(shape), source_(__alloc(this->size())), offset_(0) {}

  // enables converting std::vectors of compatible types to 1d spans
  template <typename A, typename std::enable_if_t<
                            (N == 1) && std::is_assignable_v<T&, A>, int> = 0>
  nd_span(const std::vector<A>& data) : nd_span(data.size()) {
    std::copy(data.begin(), data.end(), this->begin());
  }

  // beginning of this span (flattened)
  T* begin(void) const { return (this->source_).get() + this->offset_; }

  // end of this span (flattened)
  T* end(void) const { return this->begin() + this->size(); }

  // access a tensor, row, or element of this span as a reference
  // over the same range of memory (effectively, a sub-span)
  value_type operator[](std::size_t index) const {
    if constexpr (N == 1) {
      return *(this->begin() + index);
    } else {
      auto next_offset = (this->offset_ + index) * (this->shape_)[1];
      if constexpr (N == 2) {
        return nd_span<T, 1>(this->source_, (this->shape_)[0], next_offset);
      } else {
        nd_span<T, (N - 1)> next(this->source_, {}, next_offset);
        std::copy((this->shape_).begin() + 1, (this->shape_).end(),
                  next.shape_.begin());
        return next;
      }
    }
  }

  // access a particular element within this span
  // (most efficient way to access elements, no ARC overheads)
  template <typename... Is,
            typename std::enable_if_t<(sizeof...(Is) == N), int> = 0>
  T& operator()(Is&&... is) const {
    if constexpr (N == 1) {
      return this->operator[](((std::size_t)is)...);
    } else {
      shape_type index = {((std::size_t)is)...};
      auto offset = this->offset_;
      for (auto i = 0; i < (N - 1); i++) {
        offset = ((offset + index[i]) * this->shape_[i + 1]);
      }
      return *((this->source_).get() + offset + index[N - 1]);
    }
  }

  // returns the size of this span (flattened)
  std::size_t size(void) const {
    if constexpr (N == 1) {
      return this->shape_;
    } else {
      return std::accumulate(
          this->shape_.begin(), this->shape_.end(), 1,
          [](std::size_t x, std::size_t y) { return x * y; });
    }
  }

  // returns the shape of this span
  const shape_type& shape(void) const { return this->shape_; }

  // pups this span as a range of values
  // ( compatible with std::vector for 1D spans )
  void pup(PUP::er& p) {
    p | this->shape_;
    auto n = this->size();
    if (p.isUnpacking()) {
      this->source_.reset(__alloc(n));
    }
    PUParray(p, this->begin(), n);
  }
};

// zero-dimensional nd-spans explode upon subscripting
template <typename T>
struct nd_span<T, 0> {
  using source_type = std::shared_ptr<T>;
  using shape_type = std::array<std::size_t, 0>;
  static constexpr auto dimensionality = 0;

  template <typename... Args>
  explicit nd_span(Args&&...) {}

  T* begin(void) const { return nullptr; }

  T* end(void) const { return nullptr; }

  T& operator[](std::size_t) const { return *((T*)nullptr); }

  std::size_t size(void) const { return 0; }
};

template <typename T>
using span = nd_span<T, 1>;

namespace {
template <typename T>
struct get_span {
  using type = void;
};

template <typename T, std::size_t N>
struct get_span<nd_span<T, N>> {
  using type = T;
};
}  // namespace

template <typename T>
using get_span_t = typename get_span<T>::type;
}  // namespace ck

#endif
