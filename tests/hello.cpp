#include <ck/ck.hpp>

CK_READONLY(int, nTotal);

namespace ck {
namespace {
void __trace_begin_array_execute(ArrayElement* elt, int ep, int size) {
  envelope env;
  env.setTotalsize(size);
  env.setMsgtype(ForArrayEltMsg);
  _TRACE_CREATION_DETAILED(&env, ep);
  _TRACE_CREATION_DONE(1);
  CmiObjId projID = (elt->ckGetArrayIndex()).getProjectionID();
  _TRACE_BEGIN_EXECUTE_DETAILED(CpvAccess(curPeEvent), ForArrayEltMsg, ep,
                                CkMyPe(), 0, &projID, elt);
#if CMK_LBDB_ON
  auto id = elt->ckGetID().getElementID();
  auto& aid = elt->ckGetArrayID();
  (aid.ckLocalBranch())->recordSend(id, size, CkMyPe());
#endif
}

template <auto Entry, typename Attributes, typename Proxy, typename... Args>
auto __send(const Proxy& proxy, const CkEntryOptions* opts,
            const Args&... args) {
  using local_t = typename Proxy::local_t;
  constexpr auto is_array = Proxy::is_array;
  constexpr auto is_local = is_local_v<Entry> || contains_local_v<Attributes>;
  constexpr auto is_inline =
      is_inline_v<Entry> || contains_inline_v<Attributes>;

  if constexpr (is_local || is_inline) {
    auto* local = proxy.ckLocal();
    if (local == nullptr) {
      if constexpr (is_local) {
        CkAbort("local chare unavailable");
      }
    } else {
      using result_t =
          decltype((local->*Entry)(std::forward<const Args&>(args)...));
      if constexpr (is_inline) {
        std::size_t size;
        if constexpr (is_message_v<Args...>) {
          size = UsrToEnv(args...)->getTotalsize();
        } else {
          auto pack = std::forward_as_tuple(const_cast<Args&>(args)...);
          size = sizeof(CkMarshallMsg) + PUP::size(pack);
        }
        auto ep = get_entry_index<local_t, Entry>();
        if constexpr (is_array) {
          __trace_begin_array_execute(local, ep, size);
        } else {
          envelope env;
          env.setEpIdx(ep);
          env.setTotalsize(size);
          // TODO ( add other fields here? )
          _TRACE_CREATION_DETAILED(&env, ep);
          _TRACE_CREATION_DONE(1);
          _TRACE_BEGIN_EXECUTE(&env, local);
        }
      }

      if constexpr (is_inline || std::is_same_v<void, result_t>) {
        CkCallstackPush(static_cast<Chare*>(local));
        (local->*Entry)(std::forward<const Args&>(args)...);
        CkCallstackPop(static_cast<Chare*>(local));
        _TRACE_END_EXECUTE();
      } else {
        CkCallstackPush(static_cast<Chare*>(local));
        auto res = (local->*Entry)(std::forward<const Args&>(args)...);
        CkCallstackPop(static_cast<Chare*>(local));
        _TRACE_END_EXECUTE();
        return res;
      }
    }
  }

  if constexpr (!is_local) {
    auto* msg = ck::pack(const_cast<CkEntryOptions*>(opts),
                         std::forward<const Args&>(args)...);
    auto ep = get_entry_index<local_t, Entry>();

    if constexpr (is_array) {
      UsrToEnv(msg)->setMsgtype(ForArrayEltMsg);
      ((CkArrayMessage*)msg)->array_setIfNotThere(CkArray_IfNotThere_buffer);
    }

    proxy.send(msg, ep, message_flags_v<Entry>);
  }
}

template <auto Entry, typename Attributes, typename Proxy, std::size_t... I0s,
          std::size_t... I1s, typename... Args>
auto __send(const Proxy& proxy,
            std::index_sequence<I0s...>,    // first args
            std::index_sequence<I1s...>,    // last args
            std::tuple<const Args&...> args /* all args */
) {
  return __send<Entry, Attributes>(proxy, std::get<I0s>(std::move(args))...,
                                   std::get<I1s>(std::move(args))...);
}

template <typename T>
constexpr auto is_entry_options_v =
    std::is_same_v<CkEntryOptions*, std::remove_cv_t<std::decay_t<T>>>;
}  // namespace

template <auto Entry, typename Attributes, typename Proxy, typename... Args>
auto send(const Proxy& proxy, const Args&... args) {
  // check if the last argument is an entry options pointer
  if constexpr (is_entry_options_v<get_last_t<Args...>>) {
    return __send<Entry, Attributes>(
        proxy, std::index_sequence<sizeof...(args) - 1>{},  // put last first
        std::make_index_sequence<sizeof...(args) - 1>{},    // put first last
        std::forward_as_tuple(args...));                    // bundled args
  } else {
    return __send<Entry, Attributes>(proxy, nullptr, args...);
  }
}

template <auto Entry, typename Proxy, typename... Args>
auto send(const Proxy& proxy, const Args&... args) {
  return send<Entry, ck::attributes<>>(proxy, args...);
}
}  // namespace ck

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

  void say_hello_msg(CkDataMsg* msg);

  virtual void greet(int data) override {
    CkPrintf("%d> hello with data %d!\n", thisIndex, data);
    this->tally();
  }

  virtual void greet_greetable(std::unique_ptr<greetable>&& obj) override {
    CkPrintf("%d> hello to %s!\n", thisIndex, obj->name().c_str());
    this->tally();
  }

  int get_index(void) const { return thisIndex; }

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

void hello::say_hello_msg(CkDataMsg* msg) {
  this->greet(*((int*)msg->getData()));

  auto mine = thisIndex;
  auto right = mine + 2;

  if (right < nTotal) {
    ck::send<&hello::say_hello_msg>(thisProxy[right], msg);
  } else {
    delete msg;
  }
}

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
    auto idx = ck::send<&hello::get_index, ck::Local>(proxy[0]);
    ck::send<&hello::say_hello_msg, ck::Inline>(
        proxy[idx], CkDataMsg::buildNew(sizeof(int), &data));
  }

  void reply(std::string&& result) {
    CkPrintf("main> everyone said hello: [ %s ]\n", result.c_str());
    CkExit();
  }
};

extern "C" void CkRegisterMainModule(void) { ck::__register(); }
