#include "Cores.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cctype>

using namespace std;

Cores::Cores(int cid) : registers(32, 0), coreid(cid), pc(0) {
    registers[31] = cid;
}

int Cores::operandToReg(const string &op) {
    string tmp = op;
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    if (tmp == "cid")
        return 31;
    else // Assuming operand starts with 'x'
        return stoi(op.substr(1));
}

int Cores::execute(const PipelineInstruction &pInstr, vector<int>& mem,
                   unordered_map<string, int>& labels,
                   unordered_map<string, int>& data) {
    if (!pInstr.valid || pInstr.stalled)
        return 0;

    string opcode = pInstr.opcode;
    cout << "Core " << coreid << " executing: " << pInstr.instruction
         << " (PC = " << pInstr.pc << ")\n";

    int result = 0;
    if (opcode == "ADD" || opcode == "add") {
        result = registers[pInstr.rs1] + registers[pInstr.rs2];
        registers[pInstr.rd] = result;
        cout << "Updated X" << pInstr.rd << " = " << result << "\n";
    }
    else if (opcode == "ADDI" || opcode == "addi") {
        result = registers[pInstr.rs1] + pInstr.imm;
        registers[pInstr.rd] = result;
        cout << "Updated X" << pInstr.rd << " = " << result << "\n";
    }
    else if (opcode == "SUB" || opcode == "sub") {
        result = registers[pInstr.rs1] - registers[pInstr.rs2];
        registers[pInstr.rd] = result;
        cout << "Updated X" << pInstr.rd << " = " << result << "\n";
    }
    else if (opcode == "JAL" || opcode == "jal") {
        registers[31] = pInstr.pc + 1;
        if (labels.count(pInstr.operands[1])) {
            cout << "JAL: Jumping to " << pInstr.operands[1] << "\n";
            pc = labels[pInstr.operands[1]];
            return 1;
        }
    }
    else if (opcode == "J" || opcode == "j") {
        if (labels.count(pInstr.operands[0])) {
            cout << "J: Unconditional jump to " << pInstr.operands[0] << "\n";
            pc = labels[pInstr.operands[0]];
            return 1;
        }
    }
    else if (opcode == "BEQ" || opcode == "beq") {
        if (registers[pInstr.rs1] == registers[pInstr.rs2] && labels.count(pInstr.operands[2])) {
            cout << "BEQ: Jumping to " << pInstr.operands[2] << "\n";
            pc = labels[pInstr.operands[2]];
            return 1;
        }
    }
    else if (opcode == "BLE" || opcode == "ble") {
        if (registers[pInstr.rs1] <= registers[pInstr.rs2] && labels.count(pInstr.operands[2])) {
            cout << "BLE: Jumping to " << pInstr.operands[2] << "\n";
            pc = labels[pInstr.operands[2]];
            return 1;
        }
    }
    else if (opcode == "BNE" || opcode == "bne") {
        string label = pInstr.operands[2];
        cout << label << "\n";
        if (registers[pInstr.rs1] != registers[pInstr.rs2] && labels.count(pInstr.operands[2])) {
            cout << "BNE: Jumping to " << pInstr.operands[2] << "\n";
            pc = labels[label];
            return 1;
        }
    }
    else if (opcode == "SW" || opcode == "sw") {
        int rs = stoi(pInstr.operands[0].substr(1));  // Source register (e.g., x6)
        string offset_reg = pInstr.operands[1];       // Memory operand (e.g., "0(x5)")
        int offset;
    
        size_t open_bracket = offset_reg.find('(');
        size_t close_bracket = offset_reg.find(')');
    
        if (open_bracket != string::npos && close_bracket != string::npos) {
            // Extract offset and base register
            string offset_str = offset_reg.substr(0, open_bracket);
            string base_str = offset_reg.substr(open_bracket + 1, close_bracket - open_bracket - 1);
    
            // Handle empty offset cases (assume 0 if empty)
            offset = (offset_str.empty()) ? 0 : stoi(offset_str);
    
            // Extract base register index
            int base = stoi(base_str.substr(1));
    
            // Compute memory address
            registers[base] = mem[0];
            offset/=4;
            int mem_address = registers[base] + offset;
    
            // Debugging prints
            cout << "Base Reg: x" << base << " = " << registers[base] 
                 << ", Offset: " << offset 
                 << ", Mem Addr: " << mem_address << "\n";
    
            // Ensure memory address is aligned to 4 bytes
            if ((mem_address >= 0) && (mem_address < mem.size() * 4) ) {
                mem[mem_address] = registers[rs];  // Store value into memory
                cout << "Stored " << registers[rs] << " to Mem[" << (mem_address / 4) << "]\n";
            } else {
                cerr << "Error: Memory access unaligned at " << mem_address << "\n";
            }
        } else {
            cerr << "Error parsing SW instruction: " << pInstr.instruction << endl;
        }
    }
    
    
    else if (opcode == "LW" || opcode == "lw") {
        int rd = stoi(pInstr.operands[0].substr(1));
        string offset_reg = pInstr.operands[1];
        int offset;
    
        size_t open_bracket = offset_reg.find('(');
        size_t close_bracket = offset_reg.find(')');
    
        if (open_bracket != string::npos && close_bracket != string::npos) {
            string offset_str = offset_reg.substr(0, open_bracket);
            string base_str = offset_reg.substr(open_bracket + 1, close_bracket - open_bracket - 1);
    
            offset = stoi(offset_str);
            int base = stoi(base_str.substr(1));
    
            cout << "Base Reg: x" << base << " = " << registers[base] 
                 << ", Offset: " << offset << "\n";
    
            registers[base] = mem[0];
            offset/=4;
            int mem_address = registers[base] + offset;
    
            if ((mem_address >= 0) && (mem_address < mem.size() * 4) ) {
                registers[rd] = mem[mem_address];
                cout << "Loaded " << registers[rd] << " from Mem[" << (mem_address / 4) << "]\n";
            } else {
                cerr << "Error: Memory access out of bounds or unaligned at " << mem_address << "\n";
            }
        } else {
            cerr << "Error parsing LW instruction: " << pInstr.instruction << endl;
        }
    }
    else if (opcode == "LA" || opcode == "la") {
        if (data.count(pInstr.operands[1])) {
            registers[pInstr.rd] = data[pInstr.operands[1]];
            cout << "Loaded address " << pInstr.operands[1] << " -> X" << pInstr.rd
                 << " = " << registers[pInstr.rd] << "\n";
        }
    }
    else if (opcode == "LI" || opcode == "li") {
        registers[pInstr.rd] = stoi(pInstr.operands[1]);
        cout << "LI: Loaded immediate " << registers[pInstr.rd] << " into X" << pInstr.rd << "\n";
    }
    else if (opcode == "ECALL" || opcode == "ecall") {
        cout << "System call executed\n";
    }

    return result;
}

PipelineInstruction Cores::decode(const PipelineInstruction &pInstr) {
    PipelineInstruction decoded = pInstr;
    cout << "Core " << coreid << " decoding: " << decoded.instruction
         << " (PC = " << decoded.pc << ")\n";

    if (decoded.opcode == "ADD" || decoded.opcode == "add" ||
        decoded.opcode == "SUB" || decoded.opcode == "sub" ||
        decoded.opcode == "ADDI" || decoded.opcode == "addi" ||
        decoded.opcode == "LI" || decoded.opcode == "li" ||
        decoded.opcode == "LW" || decoded.opcode == "lw" ||
        decoded.opcode == "JAL" || decoded.opcode == "jal" ||
        decoded.opcode == "LA" || decoded.opcode == "la") {
        decoded.rd = stoi(decoded.operands[0].substr(1));
    }

    if (decoded.opcode == "ADD" || decoded.opcode == "add" ||
        decoded.opcode == "SUB" || decoded.opcode == "sub") {
        decoded.rs1 = stoi(decoded.operands[1].substr(1));
        decoded.rs2 = stoi(decoded.operands[2].substr(1));
    }
    if (decoded.opcode == "BEQ" || decoded.opcode == "beq" ||
        decoded.opcode == "BLE" || decoded.opcode == "ble" ||
        decoded.opcode == "BNE" || decoded.opcode == "bne") {
        decoded.rs1 = operandToReg(decoded.operands[0]);
        decoded.rs2 = stoi(decoded.operands[1].substr(1));
    }

    if (decoded.opcode == "ADDI" || decoded.opcode == "addi") {
        decoded.rs1 = stoi(decoded.operands[1].substr(1));
    }
    if (decoded.opcode == "ADDI" || decoded.opcode == "addi") {
        decoded.imm = stoi(pInstr.operands[2]);
    }
    
    if (decoded.rs1 != -1)
        decoded.op1 = registers[decoded.rs1];
    if (decoded.rs2 != -1)
        decoded.op2 = registers[decoded.rs2];
    return decoded;
}

PipelineInstruction Cores::memory(const PipelineInstruction &pInstr, std::vector<int>& mem) {
    PipelineInstruction m = pInstr;
    cout << "Core " << coreid << " memory: " << m.instruction 
         << " (PC = " << m.pc << ")\n";
    return m;
}

PipelineInstruction Cores::writeback(const PipelineInstruction &pInstr) {
    PipelineInstruction wb = pInstr;
    cout << "Core " << coreid << " writeback: " << wb.instruction 
         << " (PC = " << wb.pc << ")\n";
    return wb;
}

PipelineInstruction Cores::fetch(int pc, std::vector<std::string>& program) {
    PipelineInstruction fetched;
    if (pc >= static_cast<int>(program.size()))
        return fetched;
    fetched.valid = true;
    fetched.pc = pc;
    fetched.instruction = program[pc];
    std::istringstream iss(program[pc]);
    iss >> fetched.opcode;
    std::string operand;
    while (iss >> operand)
        fetched.operands.push_back(operand);
    cout << "Core " << coreid << " fetch: " << program[pc]
         << " (PC = " << pc << ")\n";
    return fetched;
}
