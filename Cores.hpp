#ifndef CORES_HPP
#define CORES_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include "PipelineInstruction.hpp"

class Cores {
public:
    std::vector<int> registers;
    int coreid;
    int pc;
    PipelineInstruction IF, ID, EX, MEM, WB;
    bool flushRequired = false;
    int newPC = 0;
    bool skipExecution = false;

    bool branch_redirect = false;
    bool fetch_stall = false;
    int branch_target = 0;

    // New synchronization variables
    static bool* sync_flags;      // Flags for each core's sync status
    static int sync_barrier_count;  // Count of cores that have reached sync point
    static const int TOTAL_CORES = 4;  // Total number of cores
    static bool sync_phase_complete;  // Indicates if sync phase is complete
    
    // Pointer to the cache hierarchy
    class CacheHierarchy* cacheHierarchy;
    Cores(int cid);
    
    int operandToReg(const std::string &op);

    PipelineInstruction fetch(int pc, std::vector<std::string>& program);
    PipelineInstruction decode(const PipelineInstruction &pInstr);
    int execute(PipelineInstruction &pInstr, std::vector<int>& mem,
                std::unordered_map<std::string, int>& labels,
                std::unordered_map<std::string, int>& data);
    PipelineInstruction memory(const PipelineInstruction &pInstr, std::vector<int>& mem);
    PipelineInstruction writeback(const PipelineInstruction &pInstr);

    int executeSPMAccess(PipelineInstruction &pInstr, int& data);

    // Cache related methods
    void setCacheHierarchy(class CacheHierarchy* cache) { cacheHierarchy = cache; }

    // Function to handle synchronization
    bool handleSync(PipelineInstruction& pInstr);

};

#endif // CORES_HPP