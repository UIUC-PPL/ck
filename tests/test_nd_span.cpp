#include <ck/ck.hpp>

#define DIRECT 0

struct main : public ck::chare<main, ck::main_chare> {
  main(CkArgMsg* msg) {
    std::array<std::size_t, 3> _3d_shape = {4, 3, 2};
    ck::nd_span<int, 3> _3d_span(_3d_shape);
    std::iota(_3d_span.begin(), _3d_span.end(), 0);

#if DIRECT
    this->receive_3d_span(std::move(_3d_span));
#else
    ck::send<&main::receive_3d_span>(thisProxy, _3d_span);
#endif

    delete msg;
  }

  void receive_3d_span(ck::nd_span<int, 3>&& span) {
    auto& shape = span.shape();
    auto status = true;
    for (auto i = 0; i < shape[0]; i++) {
      auto mat = span[i];
      for (auto j = 0; j < shape[1]; j++) {
        auto row = mat[j];
        for (auto k = 0; k < shape[2]; k++) {
          auto expected = ((i * shape[1]) + j) * shape[2] + k;
          status =
              status && (expected == span(i, j, k)) && (expected == row[k]);
        }
      }
    }

    if (status) {
      CkPrintf("3d span OK\n");
    } else {
      CkAbort("bad 3d span\n");
    }

#if DIRECT
    this->receive_2d_span(span[0]);
#else
    ck::send<&main::receive_2d_span>(thisProxy, span[0]);
#endif
  }

  void receive_2d_span(ck::nd_span<int, 2>&& span) {
    auto& shape = span.shape();
    auto status = true;
    for (auto i = 0; i < shape[0]; i++) {
      for (auto j = 0; j < shape[1]; j++) {
        auto expected = (i * shape[1]) + j;
        status = status && (expected == span(i, j));
      }
    }

    if (status) {
      CkPrintf("2d span OK\n");
    } else {
      CkAbort("bad 2d span\n");
    }

#if DIRECT
    this->receive_1d_span(span[0]);
#else
    ck::send<&main::receive_1d_span>(thisProxy, span[0]);
#endif
  }

  void receive_1d_span(ck::nd_span<int, 1>&& span) {
    auto& shape = span.shape();
    auto status = true;
    for (auto i = 0; i < shape; i++) {
      status = status && (i == span(i));
    }

    if (status) {
      CkPrintf("1d span OK\n");
    } else {
      CkAbort("bad 1d span\n");
    }

    CkExit();
  }
};

extern "C" void CkRegisterMainModule(void) { ck::__register(); }
