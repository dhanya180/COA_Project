#ifndef PIPELINE_INSTRUCTION_H
#define PIPELINE_INSTRUCTION_H

#include <string>
#include <vector>

struct PipelineInstruction {
    int coreId;
    int pc;
    std::string instruction;
    std::string opcode;
    std::vector<std::string> operands;
    bool valid = false;
    bool stalled = false; // Indicates if the instruction is stalled.
    int imm = 0;

    // For hazard detection.
    int rs1 = -1;
    int rs2 = -1;
    int rd = -1;

    // For forwarding and execution latency.
    int op1 = 0;
    int op2 = 0;
    int aluResult = 0;
    int execCyclesRemaining = 0;  // Number of execute cycles remaining.
};

#endif // PIPELINE_INSTRUCTION_H
