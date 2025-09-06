#ifndef BASE_REPLACEMENT_POLICY_H
#define BASE_REPLACEMENT_POLICY_H
#include "cache_entry.h"
#include <vector>

class BaseReplacementPolicy {
  public:
    virtual ~BaseReplacementPolicy() = default;

    virtual void invalidate(BaseReplacementData *replacementData) = 0;
    virtual void insert(BaseReplacementData *replacementData) = 0;
    virtual void access(BaseReplacementData *replacementData) = 0;
    virtual void notifyL2Eviction(BaseReplacementData *) {}
    virtual void notifyL2Insertion(BaseReplacementData *) {}
    virtual CacheEntry *getVictim(std::vector<CacheEntry> &candidates) = 0;

    virtual BaseReplacementData *initializeReplacementData() = 0;
};

#endif // BASE_REPLACEMENT_POLICY_H