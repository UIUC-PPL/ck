#ifndef CK_PUP_HPP
#define CK_PUP_HPP

#include <charm++.h>

namespace ck {

template <typename T, typename Enable = void>
struct packer;

template <typename... Args>
struct packer<std::tuple<Args...>> {
  CkMessage* operator()(CkEntryOptions* opts, Args&&... args) const {
    auto pack = std::forward_as_tuple(args...);
    auto size = PUP::size(pack);
    auto* msg = CkAllocateMarshallMsg(size, opts);
    PUP::toMemBuf(pack, (void*)msg->msgBuf, size);
    return msg;
  }
};

template <typename... Args>
CkMessage* pack(Args&&... args, CkEntryOptions* opts) {
  using tuple_t = std::tuple<std::decay_t<Args>...>;
  return packer<tuple_t>()(opts, std::forward<Args>(args)...);
}

template <typename T, typename Enable = void>
struct unpacker;

// template <typename T>
// struct unpacker<std::tuple<T*>, std::enable_if_t<std::is_base_of_v<CkMessage,
// T>>> {
//   std::tuple<T*> msg_;

//   unpacker(void* msg) : msg_((T*)msg) {}

//   std::tuple<T*>& value(void) {
//     return this->msg_;
//   }
// };

template <typename... Args>
struct unpacker<std::tuple<Args...>> {
  PUP::detail::TemporaryObjectHolder<std::tuple<Args...>> tmp_;

  unpacker(void* msg) {
    PUP::fromMem p(CkGetMsgBuffer(msg));
    p | tmp_.t;
  }

  std::tuple<Args...>& value(void) { return tmp_.t; }
};
}  // namespace ck

#endif
