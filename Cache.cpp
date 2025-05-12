#include "Cache.hpp"
#include <cmath>
#include <algorithm>
#include <random>

using namespace std;

Cache::Cache(int size, int block, int assoc, int latency, ReplacementPolicy repl, CacheType ctype)
    : cacheSize(size), blockSize(block), associativity(assoc), accessLatency(latency),
      policy(repl), type(ctype), hits(0), misses(0) {
    
    // Calculate number of sets
    numSets = cacheSize / (blockSize * associativity);
    
    // Initialize sets
    sets.reserve(numSets);
    for (int i = 0; i < numSets; i++) {
        sets.emplace_back(associativity, blockSize);
    }
    
    cout << "Created " << getTypeString() << " cache: " 
         << cacheSize << "B, " << blockSize << "B blocks, "
         << associativity << "-way, " << numSets << " sets, "
         << accessLatency << " cycle latency" << endl;
}

int Cache::getSetIndex(int address) {
    // Extract set index bits from address
    return (address / blockSize) % numSets;
}

int Cache::getTag(int address) {
    // Extract tag bits from address
    return address / (blockSize * numSets);
}

int Cache::getBlockOffset(int address) {
    // Extract block offset bits from address
    return address % blockSize;
}

int Cache::findBlockInSet(int setIndex, int tag) {
    for (int i = 0; i < associativity; i++) {
        if (sets[setIndex].blocks[i].valid && sets[setIndex].blocks[i].tag == tag) {
            return i;
        }
    }
    return -1;  // Not found
}

int Cache::selectVictim(int setIndex) {
    switch (policy) {
        case ReplacementPolicy::LRU: {
            // Get the least recently used block
            return sets[setIndex].lruList.back();
        }
        case ReplacementPolicy::FIFO: {
            // Get the first-in block
            return sets[setIndex].fifoList.front();
        }
        case ReplacementPolicy::RANDOM: {
            // Select a random block
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> distrib(0, associativity - 1);
            return distrib(gen);
        }
        default:
            return 0;  // Default to first block
    }
}

void Cache::updateReplacementInfo(int setIndex, int blockIndex) {
    // Update LRU information
    if (policy == ReplacementPolicy::LRU) {
        // Remove from current position and push to front (most recently used)
        sets[setIndex].lruList.remove(blockIndex);
        sets[setIndex].lruList.push_front(blockIndex);
    } 
    // For FIFO, only update on a miss when we insert a new block
}

bool Cache::access(int address, bool isWrite, int& accessTime, vector<int>& memory) {
    int setIndex = getSetIndex(address);
    int tag = getTag(address);
    int blockOffset = getBlockOffset(address);
    int wordOffset = blockOffset / 4;  // Assuming 4-byte words
    
    int blockIndex = findBlockInSet(setIndex, tag);
    
    // Cache hit
    if (blockIndex != -1) {
        hits++;
        accessTime = accessLatency;
        
        // Update replacement policy info
        updateReplacementInfo(setIndex, blockIndex);
        
        cout << getTypeString() << " cache HIT for address 0x" << hex << address << dec 
             << " (set=" << setIndex << ", tag=" << tag << ")" << endl;
        
        return true;
    }
    
    // Cache miss
    misses++;
    
    // Select victim for replacement
    blockIndex = selectVictim(setIndex);
    
    // Load block from memory
    int blockStartAddr = (address / blockSize) * blockSize;
    for (int i = 0; i < blockSize/4; i++) {
        int memIndex = (blockStartAddr / 4) + i;
        if (memIndex < memory.size()) {
            sets[setIndex].blocks[blockIndex].data[i] = memory[memIndex];
        }
    }
    
    // Update block info
    sets[setIndex].blocks[blockIndex].valid = true;
    sets[setIndex].blocks[blockIndex].tag = tag;
    
    // Update replacement policy info
    if (policy == ReplacementPolicy::FIFO) {
        // Remove from list and add to back (will be the next victim)
        sets[setIndex].fifoList.remove(blockIndex);
        sets[setIndex].fifoList.push_back(blockIndex);
    } else if (policy == ReplacementPolicy::LRU) {
        // For LRU, move to front as it's just been used
        sets[setIndex].lruList.remove(blockIndex);
        sets[setIndex].lruList.push_front(blockIndex);
    }
    
    cout << getTypeString() << " cache MISS for address 0x" << hex << address << dec 
         << " (set=" << setIndex << ", tag=" << tag << ")" << endl;
    
    // Return false to indicate a miss (will need to check next level)
    return false;
}

double Cache::getMissRate() {
    int totalAccesses = hits + misses;
    return totalAccesses > 0 ? static_cast<double>(misses) / totalAccesses : 0.0;
}

int Cache::getHits() {
    return hits;
}

int Cache::getMisses() {
    return misses;
}

void Cache::resetStats() {
    hits = 0;
    misses = 0;
}

void Cache::printStats() {
    cout << getTypeString() << " cache statistics:" << endl;
    cout << "  Hits: " << hits << endl;
    cout << "  Misses: " << misses << endl;
    cout << "  Miss rate: " << (getMissRate() * 100) << "%" << endl;
}
int Cache::getAccessLatency() {
    return accessLatency;
}


string Cache::getTypeString() {
    switch (type) {
        case CacheType::L1_INSTRUCTION:
            return "L1I";
        case CacheType::L1_DATA:
            return "L1D";
        case CacheType::L2_UNIFIED:
            return "L2";
        default:
            return "Unknown";
    }
}