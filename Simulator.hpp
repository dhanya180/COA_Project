#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "Cores.hpp"
#include "PipelineInstruction.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class Simulator {
public:
    std::vector<int> memory;
    int clock;
    std::vector<Cores> cores;
    std::vector<std::string> program;
    std::unordered_map<std::string, int> labels;
    std::unordered_map<std::string, int> data;
    int stallCount;
    int instructionsExecuted;
    bool forwardingEnabled; // Option to enable/disable forwarding.
    std::unordered_map<std::string, int> latencies; // User-specified latencies.

    Simulator();

    void initializeMemory();
    void parseDataSection(const std::vector<std::string>& pgm);
    void storingLabels();
    void run();
    void display();
};

#endif // SIMULATOR_H
