#ifndef CK_COMMON_HPP
#define CK_COMMON_HPP

#include <ck/span.hpp>

// copied from:
// https://www.mikeash.com/pyblog/
// friday-qa-2015-03-20-preprocessor-abuse-and-optional-parentheses.html
#define CK_PP_EXTRACT(...) CK_PP_EXTRACT __VA_ARGS__
#define CK_PP_NOTHING_CK_PP_EXTRACT
#define CK_PP_PASTE(x, ...) x##__VA_ARGS__
#define CK_PP_EVALUATING_PASTE(x, ...) CK_PP_PASTE(x, __VA_ARGS__)
#define CK_PP_UNPAREN(x) CK_PP_EVALUATING_PASTE(CK_PP_NOTHING_, CK_PP_EXTRACT x)

namespace ck {
template <typename T>
void __dummy(const T&) {}

// retrieves the EP id for a given Entry method
template <typename Class, auto Entry>
int get_entry_index(void);

// called to register all chares, readonlies,
// reducers, and pupables with the RTS
void __register(void);

// "stamps" a message with the given entry options, an
// operation that can incur up to two copies in the worst-case
CkMessage* stamp_message(CkMessage* msg, const CkEntryOptions* opts);

using register_fn_t = void (*)(void);

// TODO ( move definitions to object files? )
struct registry {
  // singleton instance of chare registry
  static std::vector<register_fn_t>& chares(void);

  // singleton instance of readonly registry
  static std::vector<register_fn_t>& readonlies(void);

  // singleton instance of reducer registry
  static std::vector<register_fn_t>& reducers(void);

  // singleton instance of pupable registry
  static std::vector<register_fn_t>& pupables(void);
};
}  // namespace ck

#endif
