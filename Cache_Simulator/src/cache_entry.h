#ifndef CACHE_ENTRY_H
#define CACHE_ENTRY_H

#include "base.h"
struct BaseReplacementData {
    bool firstTime = true;
    uint64_t address = 0;
};

class CacheEntry {
  private:
    int set;
    int way;
    uint64_t tag;
    bool valid;
    bool firstTime;
    bool prefetched;

  public:
    CacheEntry(int set, int way)
        : set(set), way(way), tag(0), valid(false), firstTime(true), prefetched(false) {}

    BaseReplacementData *replacementData;

    void setTag(uint64_t tag) { this->tag = tag; }

    void setValid(bool valid) { this->valid = valid; }

    void setFirstTime(bool firstTime) { this->firstTime = firstTime; }

    void setPrefetched(bool prefetched) { this->prefetched = prefetched; }

    int getSet() const { return set; }

    int getWay() const { return way; }

    uint64_t getTag() const { return tag; }

    bool isValid() const { return valid; }

    bool isFirstTime() const { return firstTime; }

    bool isPrefetched() const { return prefetched; }
};

#endif // CACHE_ENTRY_H