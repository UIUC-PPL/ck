#include <ck/ck.hpp>

class hello : public ck::chare<hello, int> {
  int nRecvd = 0;
  int nTotal = CkNumPes() * 4;

 public:
  void say_hello_msg(CkDataMsg* msg) {
    this->say_hello_int(*((int*)msg->getData()));

    auto mine = thisIndex;
    auto right = mine + 1;

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

class main : public ck::main_chare<main> {
 public:
  main(int argc, char** argv) {
    int data = 42;
    // create an array
    auto proxy = ck::array_proxy<hello>::create(4 * CkNumPes());
    // broadcast via parameter marshaling
    proxy.broadcast<&hello::say_hello_int>(data * 2 + 12);
    // send via conventional messaging
    proxy[0].send<&hello::say_hello_msg>(CkDataMsg::buildNew(1, &data));
  }
};
