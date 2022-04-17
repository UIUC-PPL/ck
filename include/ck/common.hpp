#ifndef CK_COMMON_HPP
#define CK_COMMON_HPP

#include <charm++.h>

namespace ck {
template <typename T>
void __dummy(const T&) {}

using register_fn_t = void (*)(void);

// TODO ( move definitions to object files? )
struct registry {
  // singleton instance of chare registry
  static std::vector<register_fn_t>& chares(void) {
    static std::vector<register_fn_t> instance;
    return instance;
  }

  // singleton instance of readonly registry
  static std::vector<register_fn_t>& readonlies(void) {
    static std::vector<register_fn_t> instance;
    return instance;
  }
};

// "stamps" a message with the given entry options, an
// operation that can incur up to two copies in the worst-case
inline CkMessage* stamp_message(CkMessage* msg, const CkEntryOptions* opts) {
  if (opts == nullptr) {
    return msg;
  }
  auto* env = UsrToEnv(msg);
  if (((opts->getPriorityBits() > env->getPriobits()) ||
       (opts->getGroupDepSize() > env->getGroupDepSize()))) {
    // pack the old message ( which may incur a copy :| )
    CkPackMessage(&env);
    // allocate a large enough new envelope
    auto* newenv = envelope::alloc(env->getMsgtype(), env->getUsersize(),
                                   opts->getPriorityBits(),
                                   GroupDepNum(opts->getGroupDepNum()));
    // copy the data to the new envelope
    CmiMemcpy(newenv, env, env->getTotalsize());
    CmiFree(env);  // dealloc the old envelope
    env = newenv;  // override the old envelope
  }
  // copy options to the envelope
  CmiMemcpy(env->getPrioPtr(), opts->getPriorityPtr(), env->getPrioBytes());
  env->setQueueing((unsigned char)opts->getQueueing());
  for (auto i = 0; i < opts->getGroupDepNum(); i++) {
    env->setGroupDep(opts->getGroupDepID(i), i);
  }
  return static_cast<CkMessage*>(EnvToUsr(env));
}
}  // namespace ck

#endif
