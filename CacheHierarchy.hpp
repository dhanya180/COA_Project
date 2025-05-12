#ifndef CACHE_HIERARCHY_HPP
#define CACHE_HIERARCHY_HPP

#include "Cache.hpp"
#include "ScratchpadMemory.hpp"
#include <vector>
#include <memory>

class CacheHierarchy {
private:
    std::unique_ptr<Cache> l1i;  // L1 Instruction Cache
    std::unique_ptr<Cache> l1d;  // L1 Data Cache
    std::unique_ptr<Cache> l2;   // L2 Unified Cache
    
    int mainMemoryLatency;
    std::unique_ptr<ScratchpadMemory> spm;
    // Statistics
    int totalMemoryAccesses;
    int totalMemoryStalls;

public:
    CacheHierarchy(
        int l1i_size, int l1i_block_size, int l1i_assoc, int l1i_latency,
        int l1d_size, int l1d_block_size, int l1d_assoc, int l1d_latency,
        int l2_size, int l2_block_size, int l2_assoc, int l2_latency,
        int mem_latency, ReplacementPolicy policy);
    
    // Memory access functions
    int accessInstructionCache(int address, std::vector<int>& memory);
    int accessDataCache(int address, bool isWrite, std::vector<int>& memory);

    int accessScratchpadMemory(int address, bool isWrite, int data, std::vector<int>& memory);
    ScratchpadMemory* getScratchpadMemory() const { return spm.get(); }
    
    // Statistics
    void printStats();
    int getTotalMemoryStalls();
    double getAverageMemoryLatency();
};

#endif // CACHE_HIERARCHY_HPP