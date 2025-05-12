#include "Cores.hpp"
#include "CacheHierarchy.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

Cores::Cores(int cid) : registers(32, 0), coreid(cid), pc(0), newPC(0), skipExecution(false), cacheHierarchy(nullptr) {
    registers[31] = cid;
}

int Cores::operandToReg(const string &op) {
    string tmp = op;
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    if(tmp == "cid") {
        return 31;
    } else if(tmp[0] == 'x') {
        // Operand starts with 'x'
        return stoi(op.substr(1));
    } else {
        // Try to parse as a direct number
        try {
            return stoi(op);
        } catch(...) {
            cerr << "Invalid register operand: " << op << endl;
            return -1;
        }
    }
}
int Cores::executeSPMAccess(PipelineInstruction &pInstr, int &data) {
    bool isStore = (pInstr.opcode == "SW_SPM");
    int idx      = pInstr.imm;           // immediate is the direct word-index
    pInstr.memoryAddress = idx;

    cout << "SPM " << (isStore ? "store" : "load")
         << " at mem[" << idx << "]\n";

    int accessTime;
    // Grab the ScratchpadMemory instance from your cache hierarchy
    auto spm = cacheHierarchy->getScratchpadMemory();

    if (isStore) {
        // For stores, pull the value to write out of pInstr.op2
        data = pInstr.op2;
        if (!spm->write(idx, data, accessTime)) {
            cerr << "SPM write OOB at mem[" << idx << "]\n";
            return 0;
        }
    } else {
        // For loads, let read() fill in `data`
        if (!spm->read(idx, data, accessTime)) {
            cerr << "SPM read OOB at mem[" << idx << "]\n";
            return 0;
        }
        // Write the loaded value back into the register file
        registers[pInstr.rd] = data;
        pInstr.aluResult     = data;
    }

    pInstr.memoryCycles = accessTime;
    cout << "→ Took " << accessTime << " cycles\n";
    return accessTime;
}


// Execute stage: when execCyclesRemaining==1, perform the operation.
int Cores::execute(PipelineInstruction &pInstr, vector<int>& mem,
    unordered_map<string, int>& labels,
    unordered_map<string, int>& data) {
    
    if (!pInstr.valid || pInstr.stalled)
        return 0;

    string opcode = pInstr.opcode;
    transform(opcode.begin(), opcode.end(), opcode.begin(), ::toupper);
    
    cout << "Core " << coreid << " executing: " << pInstr.instruction
        << " (PC = " << pInstr.pc << ")\n";

    int result = 0;
    bool branchTaken = false;

    if (opcode == "ADD") {
        result = pInstr.op1 + pInstr.op2;
        if (pInstr.rd != 0){
        registers[pInstr.rd] = result;
        cout << "Updated X" << pInstr.rd << " = " << result << "\n";
        }

    }
    else if (opcode == "ADDI") {
        result = pInstr.op1 + pInstr.imm;
        if (pInstr.rd != 0){
        registers[pInstr.rd] = result;
        cout << "Updated X" << pInstr.rd << " = " << result << "\n";
        }
    }
    else if (opcode == "SUB") {
        result = pInstr.op1 - pInstr.op2;
        if (pInstr.rd != 0){
        registers[pInstr.rd] = result;
        cout << "Updated X" << pInstr.rd << " = " << result << "\n";
        }
    }
    else if (opcode == "MUL") {
        result = pInstr.op1 * pInstr.op2;
        if (pInstr.rd != 0){
            registers[pInstr.rd] = result;
            cout << "Updated X" << pInstr.rd << " = " << result << "\n";
        }
    }
    else if (opcode == "JAL") {
        registers[pInstr.rd] = pInstr.pc + 1;
        if (labels.count(pInstr.operands[1])) {
            newPC = labels[pInstr.operands[1]];
            branchTaken = true;
            pInstr.branchTaken = true;
            cout << "JAL to " << pInstr.operands[1] << " at PC = " << newPC << "\n";
        } else {
            cerr << "ERROR: JAL label not found: " << pInstr.operands[1] << endl;
        }
    }
    else if (opcode == "J") {
        if (labels.count(pInstr.operands[0])) {
            newPC = labels[pInstr.operands[0]];
            branchTaken = true;
            pInstr.branchTaken = true;
            cout << "Jump to " << pInstr.operands[0] << " at PC = " << newPC << "\n";
        } else {
            cerr << "ERROR: Jump label not found: " << pInstr.operands[0] << endl;
        }
    }
    else if (opcode == "BEQ") {
        cout << "BEQ comparing " << pInstr.op1 << " and " << pInstr.op2 << endl;
        if (pInstr.op1 == pInstr.op2) {
            if (labels.count(pInstr.operands[2])) {
                newPC = labels[pInstr.operands[2]];
                branchTaken = true;
                pInstr.branchTaken = true;
                cout << "BEQ branch taken to " << pInstr.operands[2] << " at PC = " << newPC << "\n";
            } else {
                cerr << "ERROR: BEQ label not found: " << pInstr.operands[2] << endl;
            }
        } else {
            cout << "BEQ branch not taken\n";
        }
    }
    else if (opcode == "BLE") {
        cout << "BLE comparing " << pInstr.op1 << " <= " << pInstr.op2 << endl;
        if (pInstr.op1 <= pInstr.op2) {
            if (labels.count(pInstr.operands[2])) {
                newPC = labels[pInstr.operands[2]];
                branchTaken = true;
                pInstr.branchTaken = true;
                cout << "BLE branch taken to " << pInstr.operands[2] << " at PC = " << newPC << "\n";
            } else {
                cerr << "ERROR: BLE label not found: " << pInstr.operands[2] << endl;
            }
        } else {
            cout << "BLE branch not taken\n";
        }
    }
    else if (opcode == "BNE") {
        cout << "BNE comparing " << pInstr.op1 << " != " << pInstr.op2 << endl;
        // Check if this is a CID-based branch
        bool isCIDBranch = false;
        if (pInstr.rs1 == 31 || pInstr.operands[0] == "cid") {
            isCIDBranch = true;
            cout << "This is a CID-based branch instruction" << endl;
        }
        
        if (pInstr.op1 != pInstr.op2) {
            // Branch condition is true
            if (labels.count(pInstr.operands[2])) {
                newPC = labels[pInstr.operands[2]];
                branchTaken = true;
                pInstr.branchTaken = true;
                cout << "Core " << coreid << " - BNE branch taken to " << pInstr.operands[2] << " at PC = " << newPC << "\n";
            } else {
                cerr << "ERROR: BNE label not found: " << pInstr.operands[2] << endl;
            }
        } else {
            // Branch condition is false
            cout << "Core " << coreid << " - BNE branch not taken\n";
            
            // Special handling for CID-based branches when condition is false
            if (isCIDBranch) {
                // This core should continue execution without branching
                // But we need to mark it to skip until the branch target
                skipExecution = true;
                cout << "Core " << coreid << " will skip instructions until branch target\n";
            }
        }
    }
    else if (opcode == "SW") {
        // Step 1: Base register is logically mapped to mem[0]
        int base_mem_index = 0;

        // Step 2: Calculate offset in words
        int mem_index = base_mem_index + (pInstr.imm / 4);
        
        // Calculate physical memory address (byte addressable)
        int physAddr = pInstr.imm;
        pInstr.memoryAddress = physAddr;
        
        // Debugging Output
        cout << "Base register (x" << pInstr.rs1 << ") mapped to mem[0]" << endl;
        cout << "imm: " << pInstr.imm << ", Target Mem Index: " << mem_index << endl;

        // Step 3: Check bounds and store
        if (mem_index >= 0 && mem_index < mem.size()) {
            // Access cache for the store operation
            if (cacheHierarchy) {
                int latency = cacheHierarchy->accessDataCache(physAddr, true, mem);
                pInstr.memoryCycles = latency;
                cout << "Cache store to address 0x" << hex << physAddr << " took " << dec << latency << " cycles" << endl;
            }
            
            mem[mem_index] = pInstr.op2;
            cout << "Stored " << pInstr.op2
                << " from x" << pInstr.rs2
                << " to mem[" << mem_index << "]\n";
        } else {
            cerr << "Error: Memory access out of bounds at mem[" << mem_index << "]\n";
        }
    }
    else if (opcode == "LW") {
        // Step 1: Base register is logically mapped to mem[0]
        int base_mem_index = 0;

        // Step 2: Calculate offset in words
        int mem_index = base_mem_index + (pInstr.imm / 4);
        
        // Calculate physical memory address (byte addressable)
        int physAddr = pInstr.imm;
        pInstr.memoryAddress = physAddr;

        // Debugging Output
        cout << "Base register (x" << pInstr.rs1 << ") mapped to mem[0]" << endl;
        cout << "imm: " << pInstr.imm << ", Target Mem Index: " << mem_index << endl;

        // Step 3: Check bounds and load
        if (mem_index >= 0 && mem_index < mem.size()) {
            // Access cache for the load operation
            if (cacheHierarchy) {
                int latency = cacheHierarchy->accessDataCache(physAddr, false, mem);
                pInstr.memoryCycles = latency;
                cout << "Cache load from address 0x" << hex << physAddr << " took " << dec << latency << " cycles" << endl;
            }
            
            result = mem[mem_index];
            registers[pInstr.rd] = result;
            cout << "Loaded " << result
                << " to x" << pInstr.rd
                << " from mem[" << mem_index << "]\n";
        } else {
            cerr << "Error: Memory access out of bounds at mem[" << mem_index << "]\n";
        }
    }
    else if (opcode == "LW_SPM") {
        int loadedData = 0;
        executeSPMAccess(pInstr, loadedData);
        
        // Store loaded data to register
        if (pInstr.rd != 0) {
            registers[pInstr.rd] = loadedData;
            cout << "Loaded " << loadedData << " to x" << pInstr.rd << " from SPM\n";
        }
        result = loadedData;
    }
    else if (opcode == "SW_SPM") {
        int storeData = pInstr.op2;  // Value from source register
        executeSPMAccess(pInstr, storeData);
        
        cout << "Stored " << storeData << " from x" << pInstr.rs2 << " to SPM\n";
    }
    else if (opcode == "LA") {
        if (data.count(pInstr.operands[1])) {
            result = data[pInstr.operands[1]];
            if (pInstr.rd != 0){
            registers[pInstr.rd] = result;
            cout << "Loaded address " << pInstr.operands[1] << " -> X" << pInstr.rd << " = " << result << "\n";
            }
        } else {
            cerr << "ERROR: Data label not found: " << pInstr.operands[1] << endl;
        }
    }
    else if (opcode == "LI") {
        result = stoi(pInstr.operands[1]);
        if (pInstr.rd != 0){
        registers[pInstr.rd] = result;
        cout << "LI: Loaded immediate " << result << " into X" << pInstr.rd << "\n";
        }
    }
    else if (opcode == "ECALL") {
        cout << "System call executed\n";
    }
    
    
    pInstr.aluResult = result;
    return branchTaken ? 1 : 0;
}

// Decode stage: parse the instruction to extract register numbers.
PipelineInstruction Cores::decode(const PipelineInstruction &pInstr) {
    PipelineInstruction decoded = pInstr;
    cout << "Core " << coreid << " decoding: " << decoded.instruction
        << " (PC = " << decoded.pc << ")\n";

    // Normalize opcode to uppercase for case-insensitive comparison
    string op = decoded.opcode;
    transform(op.begin(), op.end(), op.begin(), ::toupper);
    decoded.opcode = op;
    
    // List of valid opcodes
    vector<string> validOpcodes = {
        "ADD", "SUB", "ADDI", "MUL", "LI",
        "LW", "SW", "JAL", "LA", "J",
        "BEQ", "BLE", "BNE",
        "NOP", "ECALL", "LW_SPM", "SW_SPM"
    };
    
    if (find(validOpcodes.begin(), validOpcodes.end(), op) == validOpcodes.end()) {
        cerr << "Invalid instruction: " << decoded.instruction << " (Opcode: " << op << ")\n";
        exit(1);
    }
    
    if (op == "NOP") {
        decoded.rd = -1;
        decoded.rs1 = -1;
        decoded.rs2 = -1;
        decoded.op1 = 0;
        decoded.op2 = 0;
        return decoded;
    }
    
    // Flag for control instructions
    decoded.isControlInstruction = (op == "JAL" || op == "J" || op == "BEQ" || op == "BLE" || op == "BNE");
    
    // Process instructions with destination register
    if (op == "ADD" || op == "SUB" || op == "ADDI" || op == "LI" || op == "JAL" || op == "LA" || op == "LW") {
        if (!decoded.operands.empty()) {
            decoded.rd = operandToReg(decoded.operands[0]);
        }
    }
    
    // Process R-type instructions (two source registers)
    if (op == "ADD" || op == "SUB" || op == "MUL") {
        if (decoded.operands.size() >= 3) {
            decoded.rs1 = operandToReg(decoded.operands[1]);
            decoded.rs2 = operandToReg(decoded.operands[2]);
        }
    }
    
    // Process branch instructions
    if (op == "BEQ" || op == "BLE" || op == "BNE") {
        if (decoded.operands.size() >= 3) {
            decoded.rs1 = operandToReg(decoded.operands[0]);
            decoded.rs2 = operandToReg(decoded.operands[1]);
        }
    }
    
    // Process I-type instructions (one source register + immediate)
    if (op == "ADDI") {
        if (decoded.operands.size() >= 3) {
            decoded.rs1 = operandToReg(decoded.operands[1]);
            decoded.imm = stoi(decoded.operands[2]);
        }
    }
    
    // Process memory instructions
    if (op == "LW" || op == "SW") {
        // Flag memory instructions
        decoded.isMemoryInstruction = true;
        
        // Flag load instructions separately
        if (op == "LW") {
            decoded.isLoadInstruction = true;
            decoded.rd = operandToReg(decoded.operands[0]);
        } else if (op == "SW") {
            decoded.rd = -1;  // No destination register for SW
            decoded.rs2 = operandToReg(decoded.operands[0]);
        }

        // Parse memory operand format: offset(base)
        if (decoded.operands.size() >= 2) {
            size_t open_bracket = decoded.operands[1].find('(');
            size_t close_bracket = decoded.operands[1].find(')');
            
            if (open_bracket != string::npos && close_bracket != string::npos) {
                string imm_str = decoded.operands[1].substr(0, open_bracket);
                string reg_str = decoded.operands[1].substr(open_bracket + 1, close_bracket - open_bracket - 1);
                
                try {
                    decoded.imm = stoi(imm_str);
                    decoded.rs1 = operandToReg(reg_str);
                    
                    cout << "Decoded memory operand: imm=" << decoded.imm << ", rs1=x" << decoded.rs1 << endl;
                } catch (exception& e) {
                    cerr << "ERROR: Failed to parse memory operand: " << e.what() << endl;
                }
            } else {
                cerr << "ERROR: Malformed memory operand: " << decoded.operands[1] << endl;
            }
        }
    }
    if (op == "LW_SPM" || op == "SW_SPM") {
    // Flag SPM instructions
    decoded.isMemoryInstruction = true;
    
    // Flag load instructions separately
    if (op == "LW_SPM") {
        decoded.isLoadInstruction = true;
        decoded.rd = operandToReg(decoded.operands[0]);
    } else if (op == "SW_SPM") {
        decoded.rd = -1;  // No destination register for SW_SPM
        decoded.rs2 = operandToReg(decoded.operands[0]);
    }

    // Parse memory operand format: offset(base)
    if (decoded.operands.size() >= 2) {
        // Same parsing as regular load/store
        size_t open_bracket = decoded.operands[1].find('(');
        size_t close_bracket = decoded.operands[1].find(')');
        
        if (open_bracket != string::npos && close_bracket != string::npos) {
            string imm_str = decoded.operands[1].substr(0, open_bracket);
            string reg_str = decoded.operands[1].substr(open_bracket + 1, close_bracket - open_bracket - 1);
            
            try {
                decoded.imm = stoi(imm_str);
                decoded.rs1 = operandToReg(reg_str);
                
                cout << "Decoded SPM operand: imm=" << decoded.imm << ", rs1=x" << decoded.rs1 << endl;
            } catch (exception& e) {
                cerr << "ERROR: Failed to parse SPM operand: " << e.what() << endl;
            }
        } else {
            cerr << "ERROR: Malformed SPM operand: " << decoded.operands[1] << endl;
        }
    }
}
    
    // Initialize operands with register values
    if (decoded.rs1 != -1)
        decoded.op1 = registers[decoded.rs1];
    if (decoded.rs2 != -1)
        decoded.op2 = registers[decoded.rs2];
        
    return decoded;
}

PipelineInstruction Cores::memory(const PipelineInstruction &pInstr, vector<int>& mem) {
    PipelineInstruction m = pInstr;
    cout << "Core " << coreid << " memory: " << m.instruction 
         << " (PC = " << m.pc << ")\n";
    
    // If this is a memory instruction that accessed cache, report cycles
    if (m.isMemoryInstruction && m.memoryCycles > 0) {
        cout << "Memory operation took " << m.memoryCycles << " cache cycles" << endl;
    }
    
    return m;
}

PipelineInstruction Cores::writeback(const PipelineInstruction &pInstr) {
    PipelineInstruction wb = pInstr;
    cout << "Core " << coreid << " writeback: " << wb.instruction 
         << " (PC = " << wb.pc << ")\n";
    return wb;
}

PipelineInstruction Cores::fetch(int pc, vector<string>& program) {
    PipelineInstruction fetched;
    fetched.valid = false;
    
    if (pc < 0 || pc >= program.size()) {
        cout << "Core " << coreid << " fetch: PC out of bounds " << pc << endl;
        return fetched;
    }
    
    // Access instruction cache for the current PC
    // Convert PC to a byte address (assuming 4 bytes per instruction)
    int instrAddr = pc * 4;
    
    // Check if we have a cache hierarchy
    if (cacheHierarchy) {
        // Access instruction cache
        vector<int> dummyVector;
        int iCacheLatency = cacheHierarchy->accessInstructionCache(instrAddr, dummyVector);
        cout << "Core " << coreid << " instruction fetch from address 0x" << hex << instrAddr 
             << " took " << dec << iCacheLatency << " cycles" << endl;
        fetched.memoryCycles = iCacheLatency;
    }
    
    string line = program[pc];
    fetched.pc = pc;
    fetched.instruction = line;
    
    // Skip empty lines
    if (line.empty() || line.find_first_not_of(" \t") == string::npos) {
        return fetched;
    }
    
    // Handle label-only lines like "equal:"
    size_t colonPos = line.find(':');
    if (colonPos != string::npos) {
        // Extract text after colon, if any (trim leading whitespace)
        string afterColon = line.substr(colonPos + 1);
        size_t firstNonWS = afterColon.find_first_not_of(" \t");
        
        if (firstNonWS == string::npos) {
            // Just a label - return invalid instruction
            return fetched;
        } else {
            // Label with instruction — strip the label part and use only instruction
            line = afterColon.substr(firstNonWS);
        }
    }
    
    // Now parse the instruction
    istringstream iss(line);
    iss >> fetched.opcode;
    
    // Skip comment lines
    if (fetched.opcode[0] == '#' || fetched.opcode[0] == '/' || fetched.opcode[0] == ';') {
        return fetched;
    }
    
    string operand;
    while (iss >> operand) {
        // Remove commas from operands
        if (!operand.empty() && operand.back() == ',') {
            operand.pop_back();
        }
        
        // Skip comments within the line
        if (operand[0] == '#' || operand.substr(0, 2) == "//") {
            break;
        }
        
        fetched.operands.push_back(operand);
    }
    
    fetched.valid = true;
    cout << "Core " << coreid << " fetch: " << fetched.instruction
         << " (PC = " << fetched.pc << ")\n";
    
    return fetched;
}