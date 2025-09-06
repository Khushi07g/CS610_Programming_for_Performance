#ifndef PREFETCHER_H
#define PREFETCHER_H

#include "base.h"
#include <cstdint>
#include <utility>

class Prefetcher {
  private:
    struct PrefetchTableEntry {
        bool valid;
        int64_t stride;
        uint64_t blkAddress;
        uint64_t lastAccessTime;
        uint64_t key;
    };

    int n;
    uint64_t key;
    PrefetchTableEntry *table;

    int findVictim();
    void updateState(uint64_t address);

  public:
    Prefetcher(int n);
    ~Prefetcher() { delete[] table; }

    std::pair<bool, uint64_t> getPrefetch(uint64_t address);
    void setKey(uint64_t key) { this->key = key; }
};

#endif // PREFETCHER_H