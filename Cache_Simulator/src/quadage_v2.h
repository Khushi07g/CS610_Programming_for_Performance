#ifndef QUADAGE_V2_H
#define QUADAGE_V2_H

#include "base_replacement_policy.h"

class QuadageV2 : public BaseReplacementPolicy {
  private:
    struct QuadageV2Data : BaseReplacementData {
        int age;
        bool valid;
        QuadageV2Data() : age(0), valid(false) {}
    };

  public:
    void invalidate(BaseReplacementData *replacementData) override;
    void insert(BaseReplacementData *replacementData) override;
    void access(BaseReplacementData *replacementData) override;
    CacheEntry *getVictim(std::vector<CacheEntry> &candidates) override;

    BaseReplacementData *initializeReplacementData() override;
};

#endif // QUADAGE_V2_H