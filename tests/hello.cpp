#include <ck/ck.hpp>

CK_READONLY(int, nTotal);

template <typename T>
T sum(T&& lhs, T&& rhs) {
  return lhs + ", " + rhs;
}

class hello : public ck::chare<hello, ck::array<CkIndex1D>> {
  ck::callback<std::string> reply;
  int nRecvd;

 public:
  hello(const ck::callback<std::string>& reply_) : reply(reply_), nRecvd(0) {}

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
      // takes begin..end so we need to pretend to be an array
      auto str = std::to_string(thisIndex);
      auto* msg = ck::pack_contribution(
          &str, &str + 1, ck::reducer<&sum<std::string>>(), this->reply);
      this->contribute(msg);
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
    auto reply = ck::make_callback<&main::reply>(thisProxy);
    auto proxy = ck::array_proxy<hello>::create(reply, opts);
    // broadcast via parameter marshaling
    proxy.send<&hello::say_hello_int>(data * 2 + 12);
    // send via conventional messaging
    proxy[0].send<&hello::say_hello_msg>(
        CkDataMsg::buildNew(sizeof(int), &data));
  }

  void reply(std::string&& result) {
    CkPrintf("main> everyone said hello: [ %s ]\n", result.c_str());
    CkExit();
  }
};

extern "C" void CkRegisterMainModule(void) { ck::__register(); }
