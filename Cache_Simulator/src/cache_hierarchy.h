#ifndef CACHE_HIERARCHY_H
#define CACHE_HIERARCHY_H
#include "cache.h"
#include "opt_policy.h"
#include "prefetcher.h"
#include <cstdint>
#include <map>
#include <queue>

class CacheHierarchy {
  protected:
    Cache l2;
    Cache l3;

  public:
    CacheHierarchy(int l2BlockSize, int l2Associativity, int l2NumSets,
                   BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize, int l3Associativity,
                   int l3NumSets, BaseReplacementPolicy *l3ReplacementPolicy)
        : l2(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy),
          l3(l3BlockSize, l3Associativity, l3NumSets, l3ReplacementPolicy) {}

    virtual void access(uint64_t address) = 0;

    Cache &getL2() { return l2; }
    Cache &getL3() { return l3; }
};

class InclusiveCacheHierarchy : public CacheHierarchy {
  public:
    InclusiveCacheHierarchy(int l2BlockSize, int l2Associativity, int l2NumSets,
                            BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize,
                            int l3Associativity, int l3NumSets,
                            BaseReplacementPolicy *l3ReplacementPolicy)
        : CacheHierarchy(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy, l3BlockSize,
                         l3Associativity, l3NumSets, l3ReplacementPolicy) {}

    void access(uint64_t address) override;
};

class ExclusiveCacheHierarchy : public CacheHierarchy {
  public:
    ExclusiveCacheHierarchy(int l2BlockSize, int l2Associativity, int l2NumSets,
                            BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize,
                            int l3Associativity, int l3NumSets,
                            BaseReplacementPolicy *l3ReplacementPolicy)
        : CacheHierarchy(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy, l3BlockSize,
                         l3Associativity, l3NumSets, l3ReplacementPolicy) {}

    void access(uint64_t address) override;
};

class ExclusiveV2CacheHierarchy : public CacheHierarchy {
  public:
    ExclusiveV2CacheHierarchy(int l2BlockSize, int l2Associativity, int l2NumSets,
                              BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize,
                              int l3Associativity, int l3NumSets,
                              BaseReplacementPolicy *l3ReplacementPolicy)
        : CacheHierarchy(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy, l3BlockSize,
                         l3Associativity, l3NumSets, l3ReplacementPolicy) {}

    void access(uint64_t address) override;
};

class NineCacheHierarchy : public CacheHierarchy {
  public:
    NineCacheHierarchy(int l2BlockSize, int l2Associativity, int l2NumSets,
                       BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize,
                       int l3Associativity, int l3NumSets,
                       BaseReplacementPolicy *l3ReplacementPolicy)
        : CacheHierarchy(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy, l3BlockSize,
                         l3Associativity, l3NumSets, l3ReplacementPolicy) {}

    void access(uint64_t address) override;
};

class InclusiveNotifyCacheHierarchy : public CacheHierarchy {
  public:
    InclusiveNotifyCacheHierarchy(int l2BlockSize, int l2Associativity, int l2NumSets,
                                  BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize,
                                  int l3Associativity, int l3NumSets,
                                  BaseReplacementPolicy *l3ReplacementPolicy)
        : CacheHierarchy(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy, l3BlockSize,
                         l3Associativity, l3NumSets, l3ReplacementPolicy) {}

    void access(uint64_t address) override;
};

class OptPolicyCache : public CacheHierarchy {
  public:
    OptPolicyCache(int l2BlockSize, int l2Associativity, int l2NumSets,
                   BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize, int l3Associativity,
                   int l3NumSets, BaseReplacementPolicy *l3ReplacementPolicy)
        : CacheHierarchy(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy, l3BlockSize,
                         l3Associativity, l3NumSets, l3ReplacementPolicy) {}

    void access(uint64_t address) override;
    std::map<uint64_t, std::queue<uint64_t>> &loadFutureAccesses() {
        return dynamic_cast<OptPolicy *>(getL3().getReplacementPolicy())->loadFutureAccesses();
    }
};

class PrefetchCacheHierarchy : public CacheHierarchy {
  private:
    Prefetcher *prefetcher;
    uint64_t l2Prefetches;
    uint64_t usefulL2Prefetches;
    uint64_t l3Prefetches;
    uint64_t usefulL3Prefetches;

  public:
    PrefetchCacheHierarchy(int l2BlockSize, int l2Associativity, int l2NumSets,
                           BaseReplacementPolicy *l2ReplacementPolicy, int l3BlockSize,
                           int l3Associativity, int l3NumSets,
                           BaseReplacementPolicy *l3ReplacementPolicy, Prefetcher *prefetcher)
        : CacheHierarchy(l2BlockSize, l2Associativity, l2NumSets, l2ReplacementPolicy, l3BlockSize,
                         l3Associativity, l3NumSets, l3ReplacementPolicy),
          prefetcher(prefetcher), l2Prefetches(0), usefulL2Prefetches(0), l3Prefetches(0),
          usefulL3Prefetches(0) {}

    void access(uint64_t address) override;
    void access(uint64_t address, uint64_t key) {
        prefetcher->setKey(key);
        access(address);
    }

    void prefetch(uint64_t address);

    uint64_t getL2Prefetches() const { return l2Prefetches; }
    uint64_t getL2UsefulPrefetches() const { return usefulL2Prefetches; }
    uint64_t getL3Prefetches() const { return l3Prefetches; }
    uint64_t getL3UsefulPrefetches() const { return usefulL3Prefetches; }
};

#endif // CACHE_HIERARCHY_H