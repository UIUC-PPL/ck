#ifndef CK_PUP_HPP
#define CK_PUP_HPP

#include <ck/reduction.hpp>
#include <ck/span.hpp>

namespace ck {

// helper struct to pack data into a message with options
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

// helper struct passing a message through, stamping it
// with options along the way (if specified)
template <typename T>
struct packer<std::tuple<T*>, std::enable_if_t<is_message_v<T>>> {
  CkMessage* operator()(CkEntryOptions* opts, T* t) const {
    return stamp_message(static_cast<CkMessage*>(t), opts);
  }
};

// helper struct to unpack data from a message
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

// helper struct to pass a message through
template <typename T>
struct unpacker<std::tuple<T*>, std::enable_if_t<is_message_v<T>>> {
 private:
  std::tuple<T*> msg_;

 public:
  unpacker(void* msg) : msg_(reinterpret_cast<T*>(msg)) {}

  std::tuple<T*>& value(void) { return this->msg_; }
};

// helper struct to unpack c-like main arguments from a CkArgMsg
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

// helper struct to unpack a set of values containing a bytes-like span
template <typename... Ts>
struct unpacker<std::tuple<Ts...>, std::enable_if_t<has_bytes_span_v<Ts...>>> {
 private:
  using tuple_t = std::tuple<Ts...>;
  PUP::detail::TemporaryObjectHolder<tuple_t> storage_;

  template <std::size_t I>
  void __unpack(const std::shared_ptr<CkMessage>& src, PUP::fromMem& p) {
    auto& value = std::get<I>(this->value());
    using value_t = std::decay_t<decltype(value)>;
    using T = get_span_t<value_t>;
    // if this is a span over bytes-like values...
    if constexpr (is_bytes_v<T>) {
      // unpack it using a pointer-to-offset optimization
      typename value_t::shape_type shape;
      p | shape;
      auto* data = reinterpret_cast<T*>(p.get_current_pointer());
      value = value_t(std::shared_ptr<T>(src, data), shape);
      p.advance(value.size() * sizeof(T));
    } else {
      // otherwise, just unpack it directly
      p | value;
    }
    // the ordering of this descent MUST match pup_stl.h:pup_tuple_impl
    if constexpr (I > 0) {
      __unpack<(I - 1)>(src, p);
    }
  }

 public:
  unpacker(void* msg) {
    if constexpr (sizeof...(Ts) == 1) {
      using last_t = get_last_t<Ts...>;
      if constexpr (last_t::dimensionality == 1) {
        using T = get_span_t<last_t>;
        auto msgidx = UsrToEnv(msg)->getMsgIdx();
        // exit early for contribution-like messages
        if (msgidx == CkReductionMsg::__idx) {
          this->value() = ck::unpack_contribution<T>(
              std::shared_ptr<CkReductionMsg>((CkReductionMsg*)msg));
          return;
        } else if (msgidx == CkDataMsg::__idx) {
          this->value() = ck::unpack_contribution<T>(
              std::shared_ptr<CkDataMsg>((CkDataMsg*)msg));
          return;
        }
      }
    }
    // otherwise, unpack all values recursively
    std::shared_ptr<CkMessage> src((CkMessage*)msg);
    PUP::fromMem p(CkGetMsgBuffer(msg));
    __unpack<(sizeof...(Ts) - 1)>(src, p);
  }

  tuple_t& value(void) { return (this->storage_).t; }
};

// helper function to pack data into a message with/out options
template <typename... Args>
CkMessage* pack(CkEntryOptions* opts, const Args&... args) {
  return packer<std::tuple<std::decay_t<Args>...>>()(opts, args...);
}
}  // namespace ck

#endif
