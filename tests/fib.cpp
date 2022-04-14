#include <ck/ck.hpp>

#define THRESHOLD 12

static_assert(THRESHOLD >= 3);

template <typename T>
class fib : public ck::chare<fib<T>, ck::singleton> {
  ck::callback<T> cb_;
  int nRemaining_;
  T sum_;

 public:
  static T seqFib(const T& n) {
    return (n < 2) ? n : seqFib(n - 1) + seqFib(n - 2);
  }

  fib(const T& n, ck::callback<T>&& cb)
      : cb_(std::forward<ck::callback<T>>(cb)), nRemaining_(2), sum_(0) {
    if (n < THRESHOLD) {
      this->cb_.send(seqFib(n));
    } else {
      auto reply = ck::make_callback<&fib<T>::receive_value>(this->thisProxy);
      ck::chare_proxy<fib<T>>::create(n - 1, reply);
      ck::chare_proxy<fib<T>>::create(n - 2, reply);
    }
  }

  void receive_value(const T& value) {
    this->sum_ += value;

    if (--this->nRemaining_ == 0) {
      this->cb_.send(this->sum_);
    }
  }
};

class main : public ck::chare<main, ck::main_chare> {
  std::string n_;

 public:
  main(int argc, char** argv) : n_((argc >= 2) ? argv[1] : "24") {
    auto n = std::stoull(this->n_);
    auto cb = ck::make_callback<&main::receive_value<decltype(n)>>(thisProxy);
    ck::chare_proxy<fib<decltype(n)>>::create(n, cb);
  }

  template <typename T>
  void receive_value(const T& value) {
    CkPrintf("main> fib(%s) = %s\n", this->n_.c_str(),
             std::to_string(value).c_str());
    CkExit();
  }
};

extern "C" void CkRegisterMainModule(void) { ck::__register(); }
