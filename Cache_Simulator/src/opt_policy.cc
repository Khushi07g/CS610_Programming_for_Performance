#include "opt_policy.h"
#include <sys/types.h>

void OptPolicy::access(BaseReplacementData *replacementData) {
    uint64_t blockAddr = replacementData->address / blockSize;
    uint64_t set = blockAddr % numSets;
    if (blockAddrToTime[set].find(blockAddr) != blockAddrToTime[set].end()) {
        // hit
        assert(futureAccesses[blockAddr].size() > 0);
        futureAccesses[blockAddr].pop();
        timeToBlockAddr[set].erase(blockAddrToTime[set][blockAddr]);
        blockAddrToTime[set][blockAddr] = futureAccesses[blockAddr].front();
        timeToBlockAddr[set][futureAccesses[blockAddr].front()] = blockAddr;
        return;
    }
}

void OptPolicy::insert(BaseReplacementData *replacementData) {
    uint64_t blockAddr = replacementData->address / blockSize;
    uint64_t set = blockAddr % numSets;
    assert(futureAccesses[blockAddr].size() > 0);
    futureAccesses[blockAddr].pop();
    blockAddrToTime[set][blockAddr] = futureAccesses[blockAddr].front();
    timeToBlockAddr[set][futureAccesses[blockAddr].front()] = blockAddr;
    return;
}

CacheEntry *OptPolicy::getVictim(std::vector<CacheEntry> &candidates) {
    uint64_t set = candidates[0].getSet();
    if (blockAddrToTime[set].size() < (uint64_t)associativity) {
        for (auto &entry : candidates) {
            if (!entry.isValid()) {
                return &entry;
            }
        }
    }
    
    // evict
    uint64_t victim = timeToBlockAddr[set].rbegin()->second;
    blockAddrToTime[set].erase(victim);
    timeToBlockAddr[set].erase(timeToBlockAddr[set].rbegin()->first);
    for (auto  &entry : candidates) {
        uint64_t entryBlockAddr = entry.replacementData->address / blockSize;
        if (entryBlockAddr == victim) {
            return &entry;
        }
    }
    assert(false); // should never reach here
    return nullptr;
}

BaseReplacementData *OptPolicy::initializeReplacementData() {
    return new BaseReplacementData();
}



