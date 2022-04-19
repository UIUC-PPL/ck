#ifndef CK_MAILBOX_HPP
#define CK_MAILBOX_HPP

#include <ck/pupable.hpp>

PUPbytes(CmiObjId);

namespace ck {

// TODO ( implement this on the back of the migratable threads infrastructure )
CthThread lookup_thread(const CmiObjId& tid) { return nullptr; }

template <typename T>
struct action {
 private:
  CthThread th_;
  T* t_;

 public:
  action(void) = default;

  action(CthThread th, T* t) : th_(th), t_(t) {}

  void operator()(T&& t) const {
    *(this->t_) = std::forward<T>(t);

    if (this->th_ != CthSelf()) {
      CthAwaken(this->th_);
    }
  }

  void pup(PUP::er& p) {
    if (p.isUnpacking()) {
      CmiObjId tid;
      p | tid;
      this->th_ = lookup_thread(tid);
    } else {
      // TODO ( enforce that thread is migratable )
      auto& tid = *(CthGetThreadID(this->th_));
      p | tid;
    }

    auto t = reinterpret_cast<std::uintptr_t>(this->t_);
    p | t;
    this->t_ = reinterpret_cast<T*>(t);
  }
};

template <typename T>
struct predicate : public ck::pupable<predicate<T>> {
  using parent_t = ck::pupable<predicate<T>>;

  template <typename... Args>
  explicit predicate(Args&&... args) : parent_t(std::forward<Args>(args)...) {}

  virtual bool operator()(const T& t) const = 0;
};

template <typename T>
struct request {
  using action_t = ck::action<T>;
  using predicate_t = std::unique_ptr<predicate<T>>;

 private:
  action_t action_;
  predicate_t predicate_;

 public:
  request(void) = default;

  template <typename... Args>
  request(CthThread th, T* t, Args&&... args)
      : action_(th, t), predicate_(std::forward<Args>(args)...) {}

  bool matches(const T& t) const {
    return !(this->predicate_) || ((*this->predicate_)(t));
  }

  void operator()(T&& t) const { this->action_(std::forward<T>(t)); }

  action_t& action(void) { return this->action_; }

  const action_t& action(void) const { return this->action_; }

  void pup(PUP::er& p) {
    p | this->action_;
    p | this->predicate_;
  }
};

template <typename T>
struct mailbox {
  using type = T;
  using request_t = request<type>;
  using action_t = typename request_t::action_t;
  using predicate_t = typename request_t::predicate_t;

 private:
  std::deque<T> buffer_;
  std::deque<request_t> pending_;

  auto find_matching_(const predicate_t& predicate) {
    auto end = std::end(this->buffer_);
    if ((this->buffer_).empty()) {
      return end;
    } else {
      auto search = std::begin(this->buffer_);
      for (; search != end; search++) {
        if (!predicate || (*predicate)(*search)) {
          return search;
        }
      }
      return search;
    }
  }

  auto find_matching_(const T& t) {
    auto end = std::end(this->pending_);
    if ((this->pending_).empty()) {
      return end;
    } else {
      auto search = std::begin(this->pending_);
      for (; search != end; search++) {
        if (search->matches(t)) {
          return search;
        }
      }
      return search;
    }
  }

 public:
  template <typename... Args>
  T poll(Args&&... args) {
    predicate_t predicate(std::forward<Args>(args)...);
    auto search = this->find_matching_(predicate);
    if (search != std::end(this->buffer_)) {
      auto t = std::move(*search);
      (this->buffer_).erase(search);
      return t;
    } else {
      PUP::detail::TemporaryObjectHolder<T> holder;
      (this->pending_)
          .emplace_back(CthSelf(), &(holder.t), std::move(predicate));
      CthSuspend();
      return holder.t;
    }
  }

  void post(T&& t) {
    auto search = this->find_matching_(t);
    if (search != std::end(this->pending_)) {
      (*search)(std::forward<T>(t));

      (this->pending_).erase(search);
    } else {
      (this->buffer_).emplace_back(std::forward<T>(t));
    }
  }

  void pup(PUP::er& p) {
    p | this->buffer_;
    p | this->pending_;
  }
};
}  // namespace ck

#endif
