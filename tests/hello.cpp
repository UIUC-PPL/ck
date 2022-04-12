#include <ck/ck.hpp>

class hello : public ck::chare<hello, int> {
  int nRecvd = 0;
public:
  void say_hello_msg(CkDataMsg* msg) {
    this->say_hello_int(*((int*)msg->getData()));
    delete msg;
  }

  void say_hello_int(int data) {
    CkPrintf("%d> hello with data %d!\n", CkMyPe(), data);

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
    auto proxy = ck::array_proxy<hello>::create(CkNumPes());
    // broadcast via parameter marshaling
    proxy.broadcast<&hello::say_hello_int>(data * 2 + 12);
    // broadcast via conventional messaging
    proxy.broadcast<&hello::say_hello_msg>(CkDataMsg::buildNew(1, &data));
  }
};
