#ifndef CK_PUP_HPP
#define CK_PUP_HPP

#include <ck/traits.hpp>

namespace ck {

template <typename T, typename Enable = void>
struct packer {
  template <typename... Args>
  CkMessage* operator()(CkEntryOptions* opts, const Args&... args) const {
    auto pack = std::forward_as_tuple(const_cast<Args&>(args)...);
    auto size = PUP::size(pack);
    auto* msg = CkAllocateMarshallMsg(size, opts);
    PUP::toMemBuf(pack, (void*)msg->msgBuf, size);
    return msg;
  }
};

template <typename T>
struct packer<std::tuple<T*>, std::enable_if_t<is_message_v<T>>> {
  CkMessage* operator()(CkEntryOptions* opts, T* t) const {
    // TODO ( copy entry options to message if non-null )
    return static_cast<CkMessage*>(t);
  }
};

template <typename T, typename Enable = void>
struct unpacker {
 private:
  PUP::detail::TemporaryObjectHolder<T> holder_;

 public:
  unpacker(void* msg) {
    PUP::fromMem p(CkGetMsgBuffer(msg));
    p | this->value();
  }

  T& value(void) { return (this->holder_).t; }
};

template <typename T>
struct unpacker<std::tuple<T*>, std::enable_if_t<is_message_v<T>>> {
 private:
  std::tuple<T*> msg_;

 public:
  unpacker(void* msg) : msg_(reinterpret_cast<T*>(msg)) {}

  std::tuple<T*>& value(void) { return this->msg_; }
};

template <>
struct unpacker<main_arguments_t> {
 private:
  main_arguments_t args_;

 public:
  unpacker(void* msg) {
    auto* amsg = reinterpret_cast<CkArgMsg*>(msg);
    CkEnforce(UsrToEnv(amsg)->getMsgIdx() == CMessage_CkArgMsg::__idx);
    this->args_ = std::forward_as_tuple(amsg->argc, amsg->argv);
  }

  main_arguments_t& value(void) { return this->args_; }
};

template <typename... Args>
CkMessage* pack(CkEntryOptions* opts, const Args&... args) {
  return packer<std::tuple<std::decay_t<Args>...>>()(opts, args...);
}
}  // namespace ck

#endif
