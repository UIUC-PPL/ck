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

template <typename Iterator>
CkReductionMsg* pack_contribution(const Iterator& begin, const Iterator& end,
                                  CkReduction::reducerType type) {
  using value_t = typename std::iterator_traits<Iterator>::value_type;
  constexpr auto is_bytes = is_bytes_v<value_t>;
  // determine the size of the result message
  std::size_t size;
  if constexpr (is_bytes) {
    size = (end - begin) * sizeof(value_t);
  } else {
    size = std::accumulate(begin, end, 0, [](std::size_t sz, auto& val) {
      return sz + PUP::size(const_cast<value_t&>(val));
    });
  }
  // allocate a sufficiently sized message
  auto* msg = CkReductionMsg::buildNew(size, nullptr, type, nullptr);
  auto* data = reinterpret_cast<std::byte*>(msg->getData());
  // then PUP all the accumulated values into it
  if constexpr (is_bytes) {
    std::copy(begin, end, (value_t*)data);
  } else {
    auto offset = 0;
    // PUP values individually for non-bytes types
    for (auto it = begin; it != end; it++) {
      PUP::toMem p(data + offset);
      p | const_cast<value_t&>(*(it));
      offset += p.size();
    }
    // validate that the sizing operation succeeded
    CkEnforceMsg(size == offset, "pup size mismatch!");
  }
  // return the newly formed message
  return msg;
}

template <typename Iterator>
CkReductionMsg* pack_contribution(const Iterator& begin, const Iterator& end,
                                  CkReduction::reducerType type,
                                  const CkCallback& cb) {
  auto* msg = pack_contribution(begin, end, type);
  msg->setCallback(cb);
  return msg;
}

template <typename T>
CkReductionMsg* pack_contribution(const T& value, CkReduction::reducerType type,
                                  const CkCallback& cb) {
  return pack_contribution(&value, &value + 1, type, cb);
}

template <typename T, typename Message = CkReductionMsg>
std::vector<T> unpack_contribution(Message* msg) {
  auto* data = reinterpret_cast<std::byte*>(msg->getData());
  auto size = msg->getLength();
  if constexpr (is_bytes_v<T>) {
    auto* begin = reinterpret_cast<T*>(data);
    auto* end = begin + (size / sizeof(T));
    return std::vector<T>(begin, end);
  } else {
    std::vector<T> res;
    PUP::fromMem p(data);
    while (p.size() < size) {
      PUP::detail::TemporaryObjectHolder<T> t;
      p | t;
      res.emplace_back(std::move(t.t));
    }
    CkEnforceMsg(p.size() == size, "pup size mismatch");
    return res;
  }
}

template <auto Fn>
using data_of_t = typename data_of<Fn>::type;

template <auto Fn>
struct reducer_registrar {
  static CkReduction::reducerType __type;
  static int __idx;

  using data_t = data_of_t<Fn>;
  static constexpr auto is_bytes = is_bytes_v<data_t>;

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
      auto lhs = unpack_contribution<data_t>(msgs[0]);
      auto len = lhs.size();
      // unpack all the remaining messages
      for (auto i = 1; i < nmsgs; i++) {
        auto rhs = unpack_contribution<data_t>(msgs[i]);
        CkEnforce(len == rhs.size());
        // accumulating the results into lhs
        for (auto j = 0; j < len; j++) {
          lhs[j] =
              Fn(std::forward<data_t>(lhs[j]), std::forward<data_t>(rhs[j]));
        }
      }
      return pack_contribution(lhs.begin(), lhs.end(), __type);
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
