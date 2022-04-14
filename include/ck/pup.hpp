#ifndef CK_PUP_HPP
#define CK_PUP_HPP

#include <ck/traits.hpp>

namespace ck {

template <typename T, typename Enable = void>
struct packer;

template <typename T, typename Enable = void>
struct unpacker;

template <typename T>
struct packer<std::tuple<T*>, std::enable_if_t<is_message_v<T>>> {
  CkMessage* operator()(CkEntryOptions* opts, T* t) const {
    // TODO ( copy entry options to message if non-null )
    return static_cast<CkMessage*>(t);
  }
};

template <typename... Ts>
struct packer<std::tuple<Ts...>, std::enable_if_t<!is_message_v<Ts...>>> {
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
struct unpacker<std::tuple<T*>, std::enable_if_t<is_message_v<T>>> {
  std::tuple<T*> msg_;

  unpacker(void* msg) : msg_((T*)msg) {}

  std::tuple<T*>& value(void) { return this->msg_; }
};

template <typename... Args>
struct unpacker<std::tuple<Args...>, std::enable_if_t<!is_message_v<Args...>>> {
  PUP::detail::TemporaryObjectHolder<std::tuple<Args...>> tmp_;

  unpacker(void* msg) {
    PUP::fromMem p(CkGetMsgBuffer(msg));
    p | tmp_.t;
  }

  std::tuple<Args...>& value(void) { return tmp_.t; }
};

template <typename... Args>
CkMessage* pack(CkEntryOptions* opts, const Args&... args) {
  return packer<std::tuple<std::decay_t<Args>...>>()(opts, args...);
}
}  // namespace ck

#endif
