#include <ck/ck.hpp>
#include <ck/mailbox.hpp>

#define N 16

template<typename T>
struct predicate : public ck::pupable<predicate<T>, ck::predicate<T>> {
  T j_;

  predicate(const T& j) : j_(j) {}

  virtual bool operator()(const T& i) const override {
    return (i == j_);
  }
};

struct main : public ck::chare<main, ck::main_chare> {
  // equivalent to : entry void mailbox(int);
  ck::mailbox<int> mailbox;

  main(int argc, char** argv) {
    thisProxy.send<&main::threaded>(true);
    thisProxy.send<&main::threaded>(false);
  }

  void threaded(bool send) {
    for (auto i = 0; i < N; i++) {
      if (send) /* serial */ {
        thisProxy.send<&main::mailbox>(i);

        CthYield();
      } else /* when [i] mailbox(int t) serial */ {
        auto t = mailbox.poll(new predicate(i));
        CkPrintf("%p> received %d\n", CthSelf(), t);
      }

      send = !send;
    }

    if (send) /* serial */ {
      CkExit();
    }
  }
};

CK_THREADED_ENTRY(&main::threaded);

extern "C" void CkRegisterMainModule(void) { ck::__register(); }