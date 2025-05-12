#include "ScratchpadMemory.hpp"
#include <algorithm>
#include <iomanip>

using namespace std;

ScratchpadMemory::ScratchpadMemory(int sizeBytes, int latency)
    : size(sizeBytes), wordSize(4), accessLatency(latency) {
    
    // Initialize memory (size in words)
    memory.resize(size / wordSize);

    // Fill each word with its index × 10
    for (size_t i = 0; i < memory.size(); ++i) {
        memory[i] = static_cast<int>(i) * 10;
    }
    
    cout << "Created Scratchpad Memory: " 
         << size << "B, "
         << accessLatency << " cycle latency" << endl;
}


bool ScratchpadMemory::read(int index, int& data, int& accessTime) {
    // index is now the direct word-index into memory
    if (index >= 0 && index < (int)memory.size()) {
        data = memory[index];
        accessTime = accessLatency;
        cout << "SPM read mem[" << index << "] = " << data << endl;
        return true;
    }
    cout << "ERROR: SPM read OOB at mem[" << index << "]\n";
    return false;
}

bool ScratchpadMemory::write(int index, int data, int& accessTime) {
    if (index >= 0 && index < (int)memory.size()) {
        memory[index] = data;
        accessTime = accessLatency;
        cout << "SPM write mem[" << index << "] = " << data << endl;
        return true;
    }
    cout << "ERROR: SPM write OOB at mem[" << index << "]\n";
    return false;
}


int ScratchpadMemory::getAccessLatency() const {
    return accessLatency;
}

void ScratchpadMemory::printContents() const {
    cout << "Scratchpad Memory Contents:" << endl;
    for (size_t i = 0; i < memory.size(); ++i) {
        //if (i % 4 == 0) cout << endl << "0x" << hex << (i * wordSize) << dec << ": ";
        cout << setw(2) << memory[i] << " ";
    }
    cout << endl;
}