#ifndef CK_MAIN_HPP
#define CK_MAIN_HPP

#include <ck/chare.hpp>

namespace ck {
template <typename Base>
struct chare<Base, main_chare> : public Chare, public CBase {
  using CProxy_Derived = chare_proxy<Base>;

  CBASE_MEMBERS;

  template <typename... Args>
  chare(Args&&... args) : Chare(std::forward<Args>(args)...), thisProxy(this) {
    // force the compiler to initialize this variable
    __dummy(chare_registrar<Base>::__idx);
  }

  static void __register(void) {
    constexpr auto& __idx = ck::index<Base>::__idx;

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
        CMessage_CkArgMsg::__idx, ck::index<Base>::__idx, CK_EP_NOKEEP);
    return ep;
  }

  void parent_pup(PUP::er &p) {
    recursive_pup<Base>(static_cast<Base *>(this), p);
  }
};

template <class Base>
void chare<Base, main_chare>::virtual_pup(PUP::er &p) {
  recursive_pup<Base>(static_cast<Base *>(this), p);
}
}  // namespace ck

#endif
