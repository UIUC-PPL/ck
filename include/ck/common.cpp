#include <ck/common.hpp>

namespace ck {
std::vector<register_fn_t>& registry::chares(void) {
  static std::vector<register_fn_t> instance;
  return instance;
}

std::vector<register_fn_t>& registry::readonlies(void) {
  static std::vector<register_fn_t> instance;
  return instance;
}

std::vector<register_fn_t>& registry::reducers(void) {
  static std::vector<register_fn_t> instance;
  return instance;
}

std::vector<register_fn_t>& registry::pupables(void) {
  static std::vector<register_fn_t> instance;
  return instance;
}

CkMessage* stamp_message(CkMessage* msg, const CkEntryOptions* opts) {
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

void __register(void) {
  auto& chares = registry::chares();
  for (auto& chare : chares) {
    (*chare)();
  }

  auto& readonlies = registry::readonlies();
  for (auto& readonly : readonlies) {
    (*readonly)();
  }

  auto& reducers = registry::reducers();
  for (auto& reducer : reducers) {
    (*reducer)();
  }

  auto& pupables = registry::pupables();
  for (auto& pupable : pupables) {
    (*pupable)();
  }
}
}  // namespace ck
