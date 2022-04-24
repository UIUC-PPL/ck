#ifndef CK_SPAN_HPP
#define CK_SPAN_HPP

#include <charm++.h>

#include <variant>

namespace ck {
// a range of values that can hold shared ownership of a message
template <typename T>
class span {
  T *begin_, *end_;

  using source_ptr = std::shared_ptr<T>;

  std::variant<std::monostate, source_ptr, std::vector<T>> source_;

 public:
  span(void) = default;

  span(std::vector<T>&& t)
      : begin_(t.data()),
        end_(t.data() + t.size()),
        source_(std::forward<std::vector<T>>(t)) {}

  span(std::shared_ptr<T[]>&& source, std::size_t size)
      : span(source_ptr(source, source.get()), size) {}

  span(source_ptr&& source, std::size_t size)
      : begin_(source.get()),
        end_(begin_ + size),
        source_(std::forward<source_ptr>(source)) {}

  // irrevocably takes ownership of the given source pointer
  span(T* source, std::size_t size) : span(source_ptr(source), size) {}

  T* begin(void) const { return this->begin_; }

  T* end(void) const { return this->end_; }

  T& operator[](std::size_t idx) const { return (this->begin_)[idx]; }

  std::size_t size(void) const { return this->end_ - this->begin_; }

  bool holds_source(void) const {
    return std::holds_alternative<source_ptr>(this->source_);
  }

  void pup(PUP::er& p) {
    if (p.isUnpacking()) {
      // initialize the source
      this->source_ = std::vector<T>();
      // PUP the resulting vector
      auto& data = std::get<std::vector<T>>(this->source_);
      p | data;
      // update the range
      this->begin_ = data.data();
      this->end_ = data.data() + data.size();
    } else if (std::holds_alternative<std::vector<T>>(this->source_)) {
      auto& data = std::get<std::vector<T>>(this->source_);
      p | data;
    } else {
      // this fakes pup'ing an STL container
      PUP_stl_container_size(p, *this);
      for (auto it = this->begin_; it != this->end_; it++) {
        p | (*it);
      }
    }
  }
};

namespace {
template <typename T>
struct get_span {
  using type = void;
};

template <typename T>
struct get_span<span<T>> {
  using type = T;
};
}  // namespace

template <typename T>
using get_span_t = typename get_span<T>::type;
}  // namespace ck

#endif
