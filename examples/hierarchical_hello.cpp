#include <ck/ck.hpp>

class hello_nodegroup : public ck::chare<hello_nodegroup, ck::nodegroup> {
  std::atomic<int> nDone_;
  ck::callback<void> reply_;

 public:
  hello_nodegroup(const ck::callback<void>& reply) : nDone_(0), reply_(reply) {}

  void on_group_done(void) {
    auto size = CkNodeSize(thisIndex);
    if (++(this->nDone_) >= size) {
      CkPrintf("nodegroup:%d> hello!\n", thisIndex);
      this->contribute(this->reply_);
      this->nDone_ = 0;
    }
  }
};

class hello_group : public ck::chare<hello_group, ck::group> {
  int nDone_, nLocal_;
  ck::nodegroup_proxy<hello_nodegroup> proxy_;

 public:
  hello_group(const ck::nodegroup_proxy<hello_nodegroup>& proxy)
      : nDone_(0), nLocal_(0), proxy_(proxy) {}

  void register_array(void) { this->nLocal_++; }

  void unregister_array(void) { this->nLocal_--; }

  void on_array_done(void) {
    if ((++this->nDone_) >= this->nLocal_) {
      CkPrintf("group:%d> hello!\n", thisIndex);
      ((this->proxy_).ckLocalBranch())->on_group_done();
      this->nDone_ = 0;
    }
  }
};

class hello_array : public ck::chare<hello_array, ck::array<CkIndex1D>> {
  ck::group_proxy<hello_group> proxy_;
  ck::callback<void> reply_;
  bool registered_;

  using parent_t = ck::chare<hello_array, ck::array<CkIndex1D>>;

 public:
  hello_array(CkMigrateMessage* msg) : parent_t(msg) {}

  hello_array(const ck::group_proxy<hello_group>& proxy)
      : proxy_(proxy),
        reply_(ck::make_callback<&hello_array::sayHello>(thisProxy)) {
    // enable at sync migrations
    this->usesAtSync = true;
    // simulate a migration event to:
    // - register ourself with the group
    // - contribute to the `reply' redn
    this->ckJustMigrated();
    this->ResumeFromSync();
  }

  void sayHello(void) {
    CkEnforceMsg(this->registered_, "unregistered chare!");
    CkPrintf("array:%d@pe%d> hello!\n", thisIndex, CkMyPe());
    ((this->proxy_).ckLocalBranch())->on_array_done();
    this->AtSync();
  }

  virtual void ckJustMigrated(void) override {
    ((this->proxy_).ckLocalBranch())->register_array();
    this->registered_ = true;
  }

  virtual void ckAboutToMigrate(void) override {
    ((this->proxy_).ckLocalBranch())->unregister_array();
    this->registered_ = false;
  }

  virtual void ResumeFromSync(void) override { this->contribute(this->reply_); }

  void pup(PUP::er& p) override {
    p | this->proxy_;
    p | this->reply_;
    p | this->registered_;
  }
};

class main : public ck::chare<main, ck::main_chare> {
  int phase_;
  int nPhases_;

 public:
  main(int argc, char** argv) : phase_(0) {
    auto nodedone = ck::make_callback<&main::on_nodegroup_done>(thisProxy);
    auto nodegroup = ck::create<ck::nodegroup_proxy<hello_nodegroup>>(nodedone);
    auto group = ck::create<ck::group_proxy<hello_group>>(nodegroup);
    auto factor = (argc >= 2) ? atoi(argv[1]) : 4;
    this->nPhases_ = (argc >= 3) ? atoi(argv[2]) : 2;
    CkEnforceMsg(factor > 0, "overdecomposition factor must be non-zero");
    ck::create<ck::array_proxy<hello_array>>(group, factor * CkNumPes());
  }

  void on_nodegroup_done(void) {
    CkPrintf("main> phase %d complete!\n", ++(this->phase_));
    if (this->phase_ >= this->nPhases_) {
      CkExit();
    }
  }
};

extern "C" void CkRegisterMainModule(void) { ck::__register(); }
