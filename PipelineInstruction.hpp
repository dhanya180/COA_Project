#ifndef PIPELINE_INSTRUCTION_HPP
#define PIPELINE_INSTRUCTION_HPP

#include <string>
#include <vector>

// Pipeline instruction structure.
struct PipelineInstruction {
    int coreId;
    int pc;
    std::string instruction;
    std::string opcode;
    std::vector<std::string> operands;
    bool valid = false;
    bool stalled = false; 
    int imm = 0;

    // For hazard detection.
    int rs1 = -1;
    int rs2 = -1;
    int rd = -1;

    // For forwarding and execution latency.
    int op1 = 0;
    int op2 = 0;
    int aluResult = 0;
    int execCyclesRemaining = 0;  
    
    // Flag to indicate if this is a memory instruction (for forwarding)
    bool isMemoryInstruction = false;
    bool isLoadInstruction = false; 
    // Control flow flags
    bool isControlInstruction = false;
    bool branchTaken = false;
    
    // Cache-related fields
    int memoryAddress = 0;    
    int memoryCycles = 0;      
    bool waitingForMemory = false; 
    
    // SYNC instruction specific flags
    bool isSyncInstruction = false;  
};

#endif // PIPELINE_INSTRUCTION_HPP