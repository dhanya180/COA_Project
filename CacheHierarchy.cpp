#include "CacheHierarchy.hpp"
#include <iostream>

using namespace std;

CacheHierarchy::CacheHierarchy(
    int l1i_size, int l1i_block_size, int l1i_assoc, int l1i_latency,
    int l1d_size, int l1d_block_size, int l1d_assoc, int l1d_latency,
    int l2_size, int l2_block_size, int l2_assoc, int l2_latency,
    int mem_latency, ReplacementPolicy policy)
    : mainMemoryLatency(mem_latency), totalMemoryAccesses(0), totalMemoryStalls(0) {
    
    // Initialize caches
    l1i = make_unique<Cache>(l1i_size, l1i_block_size, l1i_assoc, l1i_latency, policy, CacheType::L1_INSTRUCTION);
    l1d = make_unique<Cache>(l1d_size, l1d_block_size, l1d_assoc, l1d_latency, policy, CacheType::L1_DATA);
    l2 = make_unique<Cache>(l2_size, l2_block_size, l2_assoc, l2_latency, policy, CacheType::L2_UNIFIED);
    spm = make_unique<ScratchpadMemory>(l1d_size, l1d_latency);
    cout << "Cache hierarchy initialized with main memory latency: " << mem_latency << " cycles" << endl;
}

int CacheHierarchy::accessScratchpadMemory(int address, bool isWrite, int data, vector<int>& memory) {
    int accessTime = 0;
    bool success = false;

    if (isWrite) {
        cout << "Store to SPM address 0x" << hex << address << dec << endl;
        success = spm->write(address, data, accessTime);
    } else {
        cout << "Load from SPM address 0x" << hex << address << dec << endl;
        success = spm->read(address, data, accessTime);
    }

    if (success) {
        totalMemoryStalls += accessTime;
        return accessTime;
    } else {
        // Access failed - handle error
        cout << "SPM access failed for address 0x" << hex << address << dec << endl;
        return 0;
    }
}

int CacheHierarchy::accessInstructionCache(int address, vector<int>& memory) {
    totalMemoryAccesses++;
    int accessTime = 0;

    cout << "Instruction fetch from address 0x" << hex << address << dec << endl;

    // Try L1I first
    if (l1i->access(address, false, accessTime, memory)) {
        totalMemoryStalls += accessTime;
        return accessTime;
    }

    // L1I miss — **manually add L1I latency** and try L2
    totalMemoryStalls += l1i->getAccessLatency();
    cout << "L1I miss for address 0x" << hex << address << dec << ". Going to L2..." << endl;

    int l2AccessTime = 0;
    if (l2->access(address, false, l2AccessTime, memory)) {
        totalMemoryStalls += l2AccessTime;
        return l1i->getAccessLatency() + l2AccessTime;
    }

    // L2 miss — **manually add L2 latency** and access main memory
    totalMemoryStalls += l2->getAccessLatency();
    accessTime = l1i->getAccessLatency() + l2->getAccessLatency() + mainMemoryLatency;
    cout << "Main memory access for instruction at 0x" << hex << address << dec << endl;

    // After fetching from main memory, update L2 and L1
    int dummy = 0;
    l2->access(address, false, dummy, memory);  // Populate L2
    l1i->access(address, false, dummy, memory); // Populate L1I

    totalMemoryStalls += mainMemoryLatency;
    return accessTime;
}

int CacheHierarchy::accessDataCache(int address, bool isWrite, vector<int>& memory) {
    totalMemoryAccesses++;
    int accessTime = 0;

    cout << (isWrite ? "Store to" : "Load from") << " address 0x" << hex << address << dec << endl;

    // Try L1D first
    if (l1d->access(address, isWrite, accessTime, memory)) {
        totalMemoryStalls += accessTime;
        return accessTime;
    }

    // L1D miss — **manually add L1D latency** and try L2
    totalMemoryStalls += l1d->getAccessLatency();
    cout << "L1D miss for address 0x" << hex << address << dec << ". Going to L2..." << endl;

    int l2AccessTime = 0;
    if (l2->access(address, isWrite, l2AccessTime, memory)) {
        totalMemoryStalls += l2AccessTime;
        return l1d->getAccessLatency() + l2AccessTime;
    }

    // L2 miss — **manually add L2 latency** and access main memory
    totalMemoryStalls += l2->getAccessLatency();
    accessTime = l1d->getAccessLatency() + l2->getAccessLatency() + mainMemoryLatency;
    cout << "Main memory access for " << (isWrite ? "store" : "load") << " at 0x" << hex << address << dec << endl;

    // After fetching from main memory, update L2 and L1
    int dummy = 0;
    l2->access(address, isWrite, dummy, memory);  // Populate L2
    l1d->access(address, isWrite, dummy, memory); // Populate L1D

    totalMemoryStalls += mainMemoryLatency;
    return accessTime;
}



void CacheHierarchy::printStats() {
    cout << "\n===== CACHE HIERARCHY STATISTICS =====\n";
    l1i->printStats();
    l1d->printStats();
    l2->printStats();
    //cout << "Total memory accesses: " << totalMemoryAccesses << endl;
    cout << "Total Access Time: " << totalMemoryStalls << " Cycles" << endl;
    //cout << "Average memory access latency: " << getAverageMemoryLatency() << " cycles" << endl;
    cout << "======================================\n";
}

int CacheHierarchy::getTotalMemoryStalls() {
    return totalMemoryStalls;
}

double CacheHierarchy::getAverageMemoryLatency() {
    return totalMemoryAccesses > 0 ? 
        static_cast<double>(totalMemoryStalls) / totalMemoryAccesses : 0.0;
}