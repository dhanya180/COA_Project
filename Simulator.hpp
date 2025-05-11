#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include "Cores.hpp"
#include "CacheHierarchy.hpp"
#include "CacheConfigReader.hpp"

class Simulator {
public:
    std::vector<int> memory;
    int clock;
    std::vector<Cores> cores;
    int stallCount;
    int controlHazardCount;
    int instructionsExecuted;
    bool forwardingEnabled;
    std::vector<std::string> program;
    std::unordered_map<std::string, int> labels;
    std::unordered_map<int, bool> labelPCs;
    std::unordered_map<std::string, int> data;
    std::unordered_map<std::string, int> latencies;
    
    // Cache hierarchy
    std::unique_ptr<CacheHierarchy> cacheHierarchy;
    int memoryStalls;

    Simulator();
    void initializeMemory();
    void storingLabels();
    void parseDataSection(const std::vector<std::string>& pgm);
    void run();
    void display();
    
    // Initialize cache
    void initializeCache(const std::string& configFile);
};

#endif // SIMULATOR_HPP