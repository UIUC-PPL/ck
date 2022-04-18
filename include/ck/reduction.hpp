#ifndef CK_REDUCTION_HPP
#define CK_REDUCTION_HPP

#include <ck/pup.hpp>

namespace ck {

template <typename T>
static constexpr auto is_bytes_v = PUP::as_bytes<T>::value;

template <auto Fn>
struct data_of;

template <typename Data>
using reduction_fn_t = std::decay_t<Data> (*)(Data, Data);

template <typename Data, reduction_fn_t<Data> Fn>
struct data_of<Fn> {
  using type = std::decay_t<Data>;
};

template <auto Fn>
using data_of_t = typename data_of<Fn>::type;

template <auto Fn>
struct reducer_registrar {
  static CkReduction::reducerType __type;
  static int __idx;

  using data_t = data_of_t<Fn>;
  static constexpr auto is_bytes = is_bytes_v<data_t>;

  template <typename T>
  static void __unpack(std::vector<T>& res, CkReductionMsg* msg) {
    auto* data = reinterpret_cast<std::byte*>(msg->getData());
    auto offset = 0, size = msg->getLength();
    while (offset < size) {
      PUP::fromMem p(data + offset);
      PUP::detail::TemporaryObjectHolder<T> t;
      p | t;
      res.emplace_back(std::move(t.t));
      offset += p.size();
    }
  }

  static CkReductionMsg* __call(int nmsgs, CkReductionMsg** msgs) {
    if constexpr (is_bytes) {
      auto& rmsg = msgs[0];
      auto* rdata = reinterpret_cast<data_t*>(rmsg->getData());
      auto rlen = rmsg->getLength() / sizeof(data_t);

      for (auto i = 1; i < nmsgs; i++) {
        auto& msg = msgs[i];
        auto* data = reinterpret_cast<data_t*>(msg->getData());
        CkEnforce(msg->getLength() == rmsg->getLength());
        for (auto j = 0; j < rlen; j++) {
          rdata[j] = Fn(rdata[j], data[j]);
        }
      }

      return CkReductionMsg::buildNew(rmsg->getLength(), rdata, __type, rmsg);
    } else {
      std::vector<data_t> lhs;
      __unpack(lhs, msgs[0]);
      auto len = lhs.size();
      // unpack all the remaining messages
      for (auto i = 1; i < nmsgs; i++) {
        std::vector<data_t> rhs;
        __unpack(rhs, msgs[i]);
        CkEnforce(len == rhs.size());
        // accumulating the results into lhs
        for (auto j = 0; j < len; j++) {
          lhs[j] =
              Fn(std::forward<data_t>(lhs[j]), std::forward<data_t>(rhs[j]));
        }
      }
      // determine the size of the result message
      auto size = std::accumulate(
          lhs.begin(), lhs.end(), 0,
          [](std::size_t sz, auto& val) { return sz + PUP::size(val); });
      // allocate a sufficiently sized message
      auto* msg = CkReductionMsg::buildNew(size, nullptr, __type, nullptr);
      auto* data = reinterpret_cast<std::byte*>(msg->getData());
      auto offset = 0;
      // then PUP all the accumulated values into it
      for (auto& val : lhs) {
        PUP::toMem p(data + offset);
        p | val;
        offset += p.size();
      }
      // validate that the PUP operation succeeded
      CkEnforceMsg(size == offset, "pup size mismatch!");
      return msg;
    }
  }

  static void __register(void) {
    __type = CkReduction::addReducer(__call, is_bytes, __PRETTY_FUNCTION__);
  }
};

template <auto Fn>
int reducer_registrar<Fn>::__idx = ([](void) {
  auto& reg = registry::reducers();
  auto idx = (int)reg.size();
  reg.emplace_back(&(reducer_registrar<Fn>::__register));
  return idx;
})();

template <auto Fn>
CkReduction::reducerType reducer_registrar<Fn>::__type;

template <auto Fn>
auto reducer(void) {
  __dummy(reducer_registrar<Fn>::__idx);
  return reducer_registrar<Fn>::__type;
}
}  // namespace ck

#endif