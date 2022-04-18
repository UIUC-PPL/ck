#include "histogram.hpp"

Histogram::Histogram(int nElementsPerChare, int maxElementValue) {
  // Create a set of random values within the range [0,maxElementValue)
  myValues.resize(nElementsPerChare);
  std::srand(thisIndex);

  for (int i = 0; i < nElementsPerChare; i++) {
    myValues[i] = std::rand() % maxElementValue;
  }
}

Main::Main(int argc, char **argv) {
  if (argc != 5) {
    CkPrintf(
        "[main] Usage: pgm <nChares> <nElementsPerChare> <maxElementValue> "
        "<nBins>\n");
    CkExit(1);
  }

  // Process command-line arguments
  nChares = atoi(argv[1]);
  nElementsPerChare = atoi(argv[2]);
  maxElementValue = atoi(argv[3]);
  nBins = atoi(argv[4]);

  // Create Histogram chare array
  histogramProxy = ck::array_proxy<Histogram>::create(nElementsPerChare,
                                                      maxElementValue, nChares);
  // Create HistogramMerger group
  histogramMergerProxy = ck::group_proxy<HistogramMerger>::create(nBins);
  // set readonly
  mainProxy = thisProxy;

  // Tell Histogram chare array elements to register themselves with their
  // groups. This is done so that each local branch of the HistogramMerger group
  // knows the number of chares from which to expect submissions.
  histogramProxy.send<&Histogram::registerWithMerger>();
}

void Main::charesRegistered(void) {
  // This entry method is the target of a reduction that finishes when
  // each chare has registered itself with its local branch of the
  // HistogramMerger group

  // Now, create a number of bins
  std::vector<int> bins(nBins);
  int delta = maxElementValue / nBins;
  if (nBins * delta < maxElementValue) delta++;

  int currentKey = 0;

  // Set the key of each bin
  for (int i = 0; i < nBins; i++, currentKey += delta) bins[i] = currentKey;

  // Broadcast these bin keys to the Histogram chare array. This will cause
  // each chare to iterate through its set of values and count the number of
  // values that falls into the range implied by each bin.
  histogramProxy.send<&Histogram::count>(bins);
}

// This entry method receives the results of the histogramming operation
void Main::receiveHistogram(CkReductionMsg *msg) {
  auto binCounts = ck::unpack_contribution<int>(msg);
  auto nBins = (int)binCounts.size(), nTotalElements = 0;
  delete msg;

  // Print out number of values in each bin, check for sanity and exit
  CkPrintf("[main] histogram: \n");
  for (int i = 0; i < nBins; i++) {
    CkPrintf("bin %d count %d\n", i, binCounts[i]);
    nTotalElements += binCounts[i];
  }

  CkAssert(nTotalElements == nChares * nElementsPerChare);
  CkExit();
}

void Histogram::count(std::vector<int> &&binCounts) {
  int nKeys = binCounts.size();
  int *begin = binCounts.data();
  int *end = begin + nKeys;
  int *search = NULL;

  // Allocate an array for the histogram and initialize the counts
  std::vector<int> myCounts(nKeys, 0);

  // Iterate over values in myValues
  for (int i = 0; i < myValues.size(); i++) {
    int value = myValues[i];

    // Search for appropriate bin for value
    search = std::upper_bound(begin, end, value);
    if (search != begin) search--;
    int bin = std::distance(begin, search);

    // One more value falls in the range implied by 'bin'
    myCounts[bin]++;
  }

  // Submit partial results to HistogramMerger
  histogramMergerProxy.ckLocalBranch()->submitCounts(myCounts.data(),
                                                     myCounts.size());
}

void Histogram::registerWithMerger(void) {
  auto cb = ck::make_callback<&Main::charesRegistered>(mainProxy);
  histogramMergerProxy.ckLocalBranch()->registerMe();
  contribute(cb);
}

HistogramMerger::HistogramMerger(int nKeys) {
  // Initialize counters.
  nCharesOnMyPe = 0;
  nSubmissionsReceived = 0;

  // Allocate an array large enough to hold the counts of all bins and
  // initialize it.
  mergedCounts.resize(nKeys);
  for (int i = 0; i < mergedCounts.size(); i++) mergedCounts[i] = 0;
}

void HistogramMerger::registerMe(void) { nCharesOnMyPe++; }

// Demonstration of custom reduction registration!
template <typename T>
T sum(const T &lhs, const T &rhs) {
  return lhs + rhs;
}

// This method (also not an entry) is called by a chare on its local branch
// of the HistogramMerger group to submit a partial histogram
void HistogramMerger::submitCounts(int *binCounts, int nBins) {
  CkAssert(nBins == mergedCounts.size());
  for (int i = 0; i < nBins; i++) mergedCounts[i] += binCounts[i];

  nSubmissionsReceived++;

  // If the number of submissions received equals the number of registered
  // chares, we can contribute the partial results to a reduction that will
  // convey the final result to the main chare
  if (nSubmissionsReceived == nCharesOnMyPe) {
    auto cb = ck::make_callback<&Main::receiveHistogram>(mainProxy);
    contribute(mergedCounts.size() * sizeof(int), mergedCounts.data(),
               ck::reducer<&sum<int>>(), cb);
  }
}