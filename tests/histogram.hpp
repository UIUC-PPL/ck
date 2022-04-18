#ifndef HISTOGRAM_HPP
#define HISTOGRAM_HPP

#include <ck/ck.hpp>

class HistogramMerger : public ck::chare<HistogramMerger, ck::group> {
  int nCharesOnMyPe;
  int nSubmissionsReceived;
  std::vector<int> mergedCounts;

 public:
  HistogramMerger(int nKeys);
  void registerMe(void);
  void submitCounts(int* binCounts, int nBins);
};

class Histogram : public ck::chare<Histogram, ck::array<CkIndex1D>> {
  std::vector<int> myValues;

 public:
  Histogram(int nElementsPerChare, int maxElementValue);
  void registerWithMerger(void);
  void count(std::vector<int>&& binValues);
};

class Main : public ck::chare<Main, ck::main_chare> {
  int nChares;
  int nElementsPerChare;
  int maxElementValue;
  int nBins;

 public:
  Main(int argc, char** argv);
  void charesRegistered(void);
  void receiveHistogram(ck::span<int>&&);
};

CK_READONLY(ck::chare_proxy<Main>, mainProxy);
CK_READONLY(ck::array_proxy<Histogram>, histogramProxy);
CK_READONLY(ck::group_proxy<HistogramMerger>, histogramMergerProxy);

extern "C" void CkRegisterMainModule(void) { ck::__register(); }

#endif
