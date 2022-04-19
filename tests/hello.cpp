#include <ck/ck.hpp>

CK_READONLY(int, nTotal);

template <typename T>
T sum(T&& lhs, T&& rhs) {
  return lhs + ", " + rhs;
}

class greetable : public ck::pupable<greetable> {
  using parent_t = ck::pupable<greetable>;

 protected:
  template <typename... Args>
  greetable(Args&&... args) : parent_t(std::forward<Args>(args)...) {}

 public:
  virtual std::string name(void) = 0;
};

class greetable_person : public ck::pupable<greetable_person, greetable> {
  std::string name_;

  using parent_t = ck::pupable<greetable_person, greetable>;

 public:
  greetable_person(std::string&& name) : name_(std::move(name)) {}

  greetable_person(CkMigrateMessage* m) : parent_t(m) {}

  virtual std::string name(void) override { return "person " + this->name_; }

  virtual void pup(PUP::er& p) override {
    parent_t::pup(p);
    p | this->name_;
  }
};

class greeter : public ck::chare<greeter, ck::array<CkIndex1D>> {
 public:
  virtual void greet(int data) = 0;

  virtual void greet_greetable(std::unique_ptr<greetable>&&) = 0;
};

class hello : public ck::extends<hello, greeter> {
  ck::callback<std::string> reply;
  int nRecvd;

 public:
  hello(const ck::callback<std::string>& reply_) : reply(reply_), nRecvd(0) {}

  void say_hello_msg(CkDataMsg* msg) {
    this->greet(*((int*)msg->getData()));

    auto mine = thisIndex;
    auto right = mine + 2;

    if (right < nTotal) {
      thisProxy[right].send<&hello::say_hello_msg>(msg);
    } else {
      delete msg;
    }
  }

  virtual void greet(int data) override {
    CkPrintf("%d> hello with data %d!\n", thisIndex, data);
    this->tally();
  }

  virtual void greet_greetable(std::unique_ptr<greetable>&& obj) override {
    CkPrintf("%d> hello to %s!\n", thisIndex, obj->name().c_str());
    this->tally();
  }

 private:
  void tally(void) {
    if (++nRecvd == 3) {
      // takes begin..end so we need to pretend to be an array
      auto str = std::to_string(thisIndex);
      auto* msg = ck::pack_contribution(str, ck::reducer<&sum<std::string>>(),
                                        this->reply);
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
    proxy.send<&greeter::greet>(data * 2 + 12);
    proxy.send<&greeter::greet_greetable>(
        std::unique_ptr<greetable>(new greetable_person("bob")));
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
