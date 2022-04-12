#include <ck/ck.hpp>

class hello : public ck::chare<hello, int> {
public:
  void say_hello(void) {
    CkPrintf("%d> hello!\n", CkMyPe());

    this->contribute(CkCallback(CkCallback::ckExit));
  };
};

class main : public ck::main_chare<main> {
 public:
  main(int argc, char** argv) {
    auto proxy = ck::array_proxy<hello>::create(CkNumPes());
    proxy.broadcast<&hello::say_hello>();
  }
};
