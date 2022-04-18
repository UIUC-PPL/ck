#ifndef CK_SPAN_HPP
#define CK_SPAN_HPP

#include <charm++.h>

#include <variant>

namespace ck {
// a range of values that can be hold ownership of a message
template <typename T>
class span {
  T *begin_, *end_;

  using message_ptr = std::unique_ptr<CkMessage>;

  std::variant<std::monostate, message_ptr, std::vector<T>> source_;

 public:
  span(void) = default;

  span(std::vector<T>&& t)
      : begin_(t.data()),
        end_(t.data() + t.size()),
        source_(std::forward<std::vector<T>>(t)) {}

  template <typename... Args>
  span(T* begin, T* end, Args&&... args)
      : begin_(begin),
        end_(end),
        source_(message_ptr(std::forward<Args>(args)...)) {}

  T* begin(void) const { return this->begin_; }

  T* end(void) const { return this->end_; }

  T& operator[](std::size_t idx) const { return (this->begin_)[idx]; }

  std::size_t size(void) const { return this->end_ - this->begin_; }

  void pup(PUP::er& p) {
    if (p.isUnpacking()) {
      // initialize the source
      this->source_ = std::vector<T>();
      // PUP the resulting vector
      auto& data = std::get<std::vector<T>>(this->source_);
      p | data;
      // update the range
      this->begin_ = data.begin();
      this->end_ = data.begin() + data.size();
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
}  // namespace ck

#endif
