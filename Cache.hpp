#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <unordered_map>
#include <list>
#include <utility>
#include <string>
#include <iostream>

enum class CacheType {
    L1_INSTRUCTION,
    L1_DATA,
    L2_UNIFIED
};

enum class ReplacementPolicy {
    LRU,
    FIFO,
    RANDOM
};

struct CacheBlock {
    bool valid;
    int tag;
    std::vector<int> data;  // Actually store the data in the block
    
    CacheBlock(int blockSize) : valid(false), tag(-1), data(blockSize/4, 0) {}
};

struct CacheSet {
    std::vector<CacheBlock> blocks;
    std::list<int> lruList;  // For LRU replacement policy
    std::list<int> fifoList; // For FIFO replacement policy
    
    CacheSet(int associativity, int blockSize) : blocks(associativity, CacheBlock(blockSize)) {
        for (int i = 0; i < associativity; i++) {
            lruList.push_back(i);
            fifoList.push_back(i);
        }
    }
};

class Cache {
private:
    int cacheSize;          // Total cache size in bytes
    int blockSize;          // Block size in bytes
    int associativity;      // N-way set associative
    int accessLatency;      // Access latency in cycles
    ReplacementPolicy policy;
    CacheType type;
    
    int numSets;            // Number of sets in the cache
    std::vector<CacheSet> sets;
    
    // Cache statistics
    int hits;
    int misses;
    
    // Helper functions
    int getSetIndex(int address);
    int getTag(int address);
    int getBlockOffset(int address);
    int findBlockInSet(int setIndex, int tag);
    int selectVictim(int setIndex);
    void updateReplacementInfo(int setIndex, int blockIndex);

public:
    Cache(int size, int block, int assoc, int latency, ReplacementPolicy repl, CacheType ctype);
    
    // Cache access functions
    bool access(int address, bool isWrite, int& accessTime, std::vector<int>& memory);
    
    // Statistics
    double getMissRate();
    int getHits();
    int getMisses();
    void resetStats();
    
    // Debug/display functions
    void printStats();
    std::string getTypeString();
};

#endif // CACHE_HPP