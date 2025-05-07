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

    Cores(int cid);
    
    int operandToReg(const std::string &op);

    // Execute stage: when execCyclesRemaining==1, perform the operation.
    int execute(PipelineInstruction &pInstr, std::vector<int>& mem,
        std::unordered_map<std::string, int>& labels,
        std::unordered_map<std::string, int>& data);

    // Decode stage: parse the instruction to extract register numbers.
    PipelineInstruction decode(const PipelineInstruction &pInstr);

    PipelineInstruction memory(const PipelineInstruction &pInstr, std::vector<int>& mem);

    PipelineInstruction writeback(const PipelineInstruction &pInstr);

    PipelineInstruction fetch(int pc, std::vector<std::string>& program);
};

#endif // CORES_HPP