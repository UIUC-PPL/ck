#ifndef CK_MAIN_HPP
#define CK_MAIN_HPP

#include <ck/proxy.hpp>

namespace ck {
template <typename Base>
struct main_chare : public Chare {
  static int& __idx;

  chare_proxy<Base> thisProxy;

  template <typename... Args>
  main_chare(Args&&... args)
      : Chare(std::forward<Args>(args)...), thisProxy(this) {
    // force the compiler to initialize this variable
    __dummy(chare_registrar<Base>::__idx);
  }

  static void __register(void) {
    __idx = CkRegisterChare(__PRETTY_FUNCTION__, sizeof(Base), TypeMainChare);
    CkRegisterBase(__idx, CkIndex_Chare::__idx);
    CkRegisterMainChare(__idx, __idx_main_CkArgMsg());

    auto& entries = ck::index<Base>::__entries();
    for (auto& entry : entries) {
      (*entry)();
    }
  }

  static int __idx_main_CkArgMsg(void) {
    static int ep = CkRegisterEp(
        __PRETTY_FUNCTION__,
        [](void* msg, void* obj) {
          auto* amsg = (CkArgMsg*)msg;
          new ((Base*)obj) Base(amsg->argc, amsg->argv);
        },
        CMessage_CkArgMsg::__idx, __idx, CK_EP_NOKEEP);
    return ep;
  }
};

template <typename Base>
int& main_chare<Base>::__idx = chare_registrar<Base>::__idx;
};  // namespace ck

#endif
