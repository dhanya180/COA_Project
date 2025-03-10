#ifndef CORES_H
#define CORES_H

#include "PipelineInstruction.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class Cores {
public:
    std::vector<int> registers;
    int coreid;
    int pc;

    Cores(int cid);

    // Converts an operand (e.g., "x5" or "cid") to a register number.
    int operandToReg(const std::string &op);

    // Execute the instruction in the execute stage.
    int execute(const PipelineInstruction &pInstr, std::vector<int>& mem,
                std::unordered_map<std::string, int>& labels,
                std::unordered_map<std::string, int>& data);

    // Decode the instruction and fill in register numbers.
    PipelineInstruction decode(const PipelineInstruction &pInstr);

    // Memory stage.
    PipelineInstruction memory(const PipelineInstruction &pInstr, std::vector<int>& mem);

    // Writeback stage.
    PipelineInstruction writeback(const PipelineInstruction &pInstr);

    // Fetch stage.
    PipelineInstruction fetch(int pc, std::vector<std::string>& program);
};

#endif // CORES_H
