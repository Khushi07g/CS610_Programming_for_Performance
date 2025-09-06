#include "quadage_v2.h"

void QuadageV2::invalidate(BaseReplacementData *replacementData) {
    static_cast<QuadageV2Data *>(replacementData)->valid = false;
}

void QuadageV2::insert(BaseReplacementData *replacementData) {
    static_cast<QuadageV2Data *>(replacementData)->valid = true;
    if (static_cast<QuadageV2Data *>(replacementData)->firstTime) {
        static_cast<QuadageV2Data *>(replacementData)->age = 2;
    } else {
        static_cast<QuadageV2Data *>(replacementData)->age = 0;
    }
}

void QuadageV2::access(BaseReplacementData *replacementData) {
    assert(static_cast<QuadageV2Data *>(replacementData)->valid);
    static_cast<QuadageV2Data *>(replacementData)->age = 0;
}

// assumes candidates are sorted by way ID
CacheEntry *QuadageV2::getVictim(std::vector<CacheEntry> &candidates) {
    for (auto &candidate : candidates) {
        QuadageV2Data *data = static_cast<QuadageV2Data *>(candidate.replacementData);
        if (!data->valid) {
            return &candidate;
        }
    }
    while (true) {
        for (auto &candidate : candidates) {
            QuadageV2Data *data = static_cast<QuadageV2Data *>(candidate.replacementData);
            if (data->age == 3) {
                return &candidate;
            }
        }
        for (auto &candidate : candidates) {
            QuadageV2Data *data = static_cast<QuadageV2Data *>(candidate.replacementData);
            data->age = std::min(data->age + 1, 3);
        }
    }
}

BaseReplacementData *QuadageV2::initializeReplacementData() { return new QuadageV2Data(); }