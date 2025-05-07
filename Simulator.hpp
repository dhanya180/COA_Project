#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include "Cores.hpp"

class Simulator {
public:
    std::vector<int> memory;
    int clock;
    std::vector<Cores> cores;
    std::vector<std::string> program;
    std::unordered_map<std::string,int> labels;
    std::unordered_map<int, bool> labelPCs;
    std::unordered_map<std::string,int> data;
    int stallCount;
    int controlHazardCount;
    int instructionsExecuted;
    bool forwardingEnabled; // option to enable/disable forwarding
    std::unordered_map<std::string,int> latencies; // user-specified latencies for arithmetic instructions

    Simulator();

    void initializeMemory();

    void parseDataSection(const std::vector<std::string>& pgm);

    void storingLabels();

    void run();

    void display();
};

#endif // SIMULATOR_HPP