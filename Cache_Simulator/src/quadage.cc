#include "quadage.h"

void Quadage::invalidate(BaseReplacementData *replacementData) {
    static_cast<QuadageData *>(replacementData)->valid = false;
    static_cast<QuadageData *>(replacementData)->inParent = false;
}

void Quadage::insert(BaseReplacementData *replacementData) {
    static_cast<QuadageData *>(replacementData)->valid = true;
    static_cast<QuadageData *>(replacementData)->age = 2;
    static_cast<QuadageData *>(replacementData)->inParent = false;
}

void Quadage::access(BaseReplacementData *replacementData) {
    assert(static_cast<QuadageData *>(replacementData)->valid);
    static_cast<QuadageData *>(replacementData)->age = 0;
}

// assumes candidates are sorted by way ID
CacheEntry *Quadage::getVictim(std::vector<CacheEntry> &candidates) {
    for (auto &candidate : candidates) {
        QuadageData *data = static_cast<QuadageData *>(candidate.replacementData);
        if (!data->valid) {
            return &candidate;
        }
    }
    while (true) {
        for (auto &candidate : candidates) {
            QuadageData *data = static_cast<QuadageData *>(candidate.replacementData);
            if (data->age == 3) {
                return &candidate;
            }
        }
        for (auto &candidate : candidates) {
            QuadageData *data = static_cast<QuadageData *>(candidate.replacementData);
            data->age = std::min(data->age + 1, 3);
        }
    }
}

BaseReplacementData *Quadage::initializeReplacementData() { return new QuadageData(); }

CacheEntry *QuadageNotify::getVictim(std::vector<CacheEntry> &candidates) {
    for (auto &candidate : candidates) {
        QuadageData *data = static_cast<QuadageData *>(candidate.replacementData);
        if (!data->valid) {
            return &candidate;
        }
    }
    while (true) {
        for (auto &candidate : candidates) {
            QuadageData *data = static_cast<QuadageData *>(candidate.replacementData);
            if (data->age == 3 && !data->inParent) {
                return &candidate;
            }
        }
        for (auto &candidate : candidates) {
            QuadageData *data = static_cast<QuadageData *>(candidate.replacementData);
            if (data->age == 3) {
                return &candidate;
            }
        }
        for (const auto &candidate : candidates) {
            QuadageData *data = static_cast<QuadageData *>(candidate.replacementData);
            data->age = std::min(data->age + 1, 3);
        }
    }
}

void QuadageNotify::notifyL2Eviction(BaseReplacementData *replacementData) {
    QuadageData *data = static_cast<QuadageData *>(replacementData);
    assert(data->inParent);
    data->inParent = false;
}

void QuadageNotify::notifyL2Insertion(BaseReplacementData *replacementData) {
    QuadageData *data = static_cast<QuadageData *>(replacementData);
    assert(!data->inParent);
    data->inParent = true;
}