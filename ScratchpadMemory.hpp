#ifndef SCRATCHPAD_MEMORY_HPP
#define SCRATCHPAD_MEMORY_HPP

#include <vector>
#include <iostream>

class ScratchpadMemory {
private:
    std::vector<int> memory;
    int size;      // Size in bytes
    int wordSize;  // Size of a word in bytes (4)
    int accessLatency;

public:
    ScratchpadMemory(int sizeBytes, int latency);
    
    bool read(int address, int& data, int& accessTime);
    bool write(int address, int data, int& accessTime);
    int getAccessLatency() const;
    
    // For debugging
    void printContents() const;
};

#endif // SCRATCHPAD_MEMORY_HPP