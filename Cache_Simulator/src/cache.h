#ifndef CACHE_H
#define CACHE_H

#include "base_replacement_policy.h"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

class Cache {
  private:
    const int blockSize;
    const int associativity;
    const int numSets;
    std::vector<std::vector<CacheEntry>> cache;
    std::vector<std::unordered_map<uint64_t, int>> tagToWayMap;
    std::unordered_set<uint64_t> visited;
    BaseReplacementPolicy *replacementPolicy;
    uint64_t misses;
    uint64_t coldMisses;

  public:
    Cache(int blockSize, int associativity, int numSets, BaseReplacementPolicy *replacementPolicy);

    bool access(uint64_t address);
    void updateTagToWay(int set, uint64_t tag, int way);

    bool evict(uint64_t address);

    bool isPresent(uint64_t address);

    CacheEntry insert(uint64_t address);

    CacheEntry insert(uint64_t address, bool firstTime);

    std::vector<CacheEntry> &getEntries(uint64_t address);

    int getNumSets() { return numSets; }

    int getBlockSize() { return blockSize; }

    uint64_t getMisses() { return misses; }

    BaseReplacementPolicy *getReplacementPolicy() { return replacementPolicy; }

    void notifyL2Eviction(uint64_t address);

    void notifyL2Insertion(uint64_t address);

    uint64_t getColdMisses() const { return coldMisses; }

    bool isPrefetched(uint64_t address);

    void setPrefetched(uint64_t address, bool prefetched);
};

#endif // CACHE_H