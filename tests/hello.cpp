#include <ck/ck.hpp>

CK_READONLY(int, nTotal);

class hello : public ck::chare<hello, ck::array<CkIndex1D>> {
  int nRecvd = 0;

 public:
  void say_hello_msg(CkDataMsg* msg) {
    this->say_hello_int(*((int*)msg->getData()));

    auto mine = thisIndex;
    auto right = mine + 2;

    if (right < nTotal) {
      thisProxy[right].send<&hello::say_hello_msg>(msg);
    } else {
      delete msg;
    }
  }

  void say_hello_int(int data) {
    CkPrintf("%d> hello with data %d!\n", thisIndex, data);

    if (++nRecvd == 2) {
      this->contribute(CkCallback(CkCallback::ckExit));
    }
  }
};

// calls with perfect forwarding when its available
CK_INLINE_ENTRY(&hello::say_hello_msg);

class main : public ck::chare<main, ck::main_chare> {
 public:
  main(int argc, char** argv) {
    int data = 42;
    // creates an array with the even indices from 0..(4*CkNumPes)
    nTotal = 4 * CkNumPes();
    ck::constructor_options<hello> opts(0, nTotal, 2);
    auto proxy = ck::array_proxy<hello>::create(opts);
    // broadcast via parameter marshaling
    proxy.send<&hello::say_hello_int>(data * 2 + 12);
    // send via conventional messaging
    proxy[0].send<&hello::say_hello_msg>(CkDataMsg::buildNew(1, &data));
  }
};

extern "C" void CkRegisterMainModule(void) { ck::__register(); }
