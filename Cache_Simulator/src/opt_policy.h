#ifndef OPT_POLICY_H
#define OPT_POLICY_H
#include "base_replacement_policy.h"
#include <cstdint>
#include <map>
#include <queue>
#include <unordered_set>


class OptPolicy : public BaseReplacementPolicy {
  private:
    const int blockSize;
    const int associativity;
    const int numSets;
    std::map<uint64_t, std::queue<uint64_t>> futureAccesses;
    std::map<uint64_t, std::map<uint64_t, uint64_t>> timeToBlockAddr;
    std::map<uint64_t, std::map<uint64_t, uint64_t>> blockAddrToTime;
    std::unordered_set<uint64_t> visited;
    uint64_t coldMisses;
    uint64_t capacityMisses;
  public:
    OptPolicy(int blockSize, int associativity, int numSets)
        : blockSize(blockSize), associativity(associativity), numSets(numSets),
          coldMisses(0), capacityMisses(0) {}

    void access(BaseReplacementData *replacementData) override;
    void insert(BaseReplacementData *replacementData) override;
    CacheEntry *getVictim(std::vector<CacheEntry> &candidates) override;
    void invalidate(BaseReplacementData *) override {}
    BaseReplacementData *initializeReplacementData() override;

    uint64_t getColdMisses() const { return coldMisses; }
    uint64_t getCapacityMisses() const { return capacityMisses; }
    std::map<uint64_t, std::queue<uint64_t>>& loadFutureAccesses() { return futureAccesses; }
};

#endif