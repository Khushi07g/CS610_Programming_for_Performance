#ifndef QUADAGE_H
#define QUADAGE_H
#include "base_replacement_policy.h"

class Quadage : public BaseReplacementPolicy {
  protected:
    struct QuadageData : BaseReplacementData {
        int age;
        bool valid;
        bool inParent;
        QuadageData() : age(0), valid(false), inParent(false) {}
    };

  public:
    void invalidate(BaseReplacementData *replacementData) override;
    void insert(BaseReplacementData *replacementData) override;
    void access(BaseReplacementData *replacementData) override;
    CacheEntry *getVictim(std::vector<CacheEntry> &candidates) override;

    BaseReplacementData *initializeReplacementData() override;
};

class QuadageNotify : public Quadage {
  public:
    CacheEntry *getVictim(std::vector<CacheEntry> &candidates) override;
    void notifyL2Eviction(BaseReplacementData *replacementData) override;
    void notifyL2Insertion(BaseReplacementData *replacementData) override;
};

#endif // QUADAGE_H