#ifndef CK_CALLBACK_HPP
#define CK_CALLBACK_HPP

#include <ck/registrar.hpp>

namespace ck {

template <typename... Ts>
struct callback : public CkCallback {
  template <typename... Args>
  callback(Args&&... args) : CkCallback(std::forward<Args>(args)...) {}

  template <typename... Args>
  void send(Args&&... args) const {
    if constexpr (is_message_v<std::decay_t<Args>...>) {
      CkCallback::send(args...);
    } else {
      CkCallback::send(ck::pack(nullptr, std::forward<Args>(args)...));
    }
  }
};

namespace {
template <typename T>
struct __callback;

template <typename... Ts>
struct __callback<std::tuple<Ts...>> {
  using type = callback<std::decay_t<Ts>...>;
};
}  // namespace

template <auto Entry, typename Proxy>
auto make_callback(const Proxy& proxy) {
  using local_t = typename Proxy::local_t;
  using arguments_t = method_arguments_t<Entry>;
  using callback_t = typename __callback<arguments_t>::type;
  auto ep = ck::index<local_t>::template method_index<Entry>();
  return callback_t(ep, proxy);
}
}  // namespace ck

#endif
