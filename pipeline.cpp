#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <unordered_map>

using namespace std;

// Structure to hold the instruction as it moves through the pipeline
struct PipelineInstruction {
    int coreId;
    int pc;
    string instruction;
    string opcode;
    vector<string> operands;
    bool valid = false;
};

class Cores {
public:
    vector<int> registers;
    int coreid;
    int pc;

    Cores(int cid) : registers(32, 0), coreid(cid), pc(0) {}

    bool execute(const PipelineInstruction& pInstr, vector<int>& mem, unordered_map<string, int>& labels, unordered_map<string, int>& data) {
        if (!pInstr.valid) return false;
        string opcode = pInstr.opcode;

        cout << "Core " << coreid << " executing: " << pInstr.instruction << " (PC = " << pInstr.pc << ")\n";

        if (opcode == "ADD" || opcode == "add") {
            int rd = stoi(pInstr.operands[0].substr(1));
            int rs1 = stoi(pInstr.operands[1].substr(1));
            int rs2 = stoi(pInstr.operands[2].substr(1));
            registers[rd] = registers[rs1] + registers[rs2];
            cout << "Updated X" << rd << " = " << registers[rd] << "\n";
        }

        else if (opcode == "ADDI" || opcode == "addi") {
            int rd = stoi(pInstr.operands[0].substr(1));
            int rs1 = stoi(pInstr.operands[1].substr(1));
            int imm = stoi(pInstr.operands[2]);
            registers[rd] = registers[rs1] + imm;
            cout << "Updated X" << rd << " = " << registers[rd] << "\n";
        }

        else if (opcode == "SUB" || opcode == "sub") {
            int rd = stoi(pInstr.operands[0].substr(1));
            int rs1 = stoi(pInstr.operands[1].substr(1));
            int rs2 = stoi(pInstr.operands[2].substr(1));
            registers[rd] = registers[rs1] - registers[rs2];
            cout << "Updated X" << rd << " = " << registers[rd] << "\n";
        }

        else if (opcode == "JAL" || opcode == "jal") {
            int rd = stoi(pInstr.operands[0].substr(1));
            string label = pInstr.operands[1];
            if (labels.count(label)) {
                cout << "JAL: Jumping to " << label << "\n";
                registers[31] = pInstr.pc + 1;
                pc = labels[label]; // Update core's PC
                return true;
            }
        }

        else if (opcode == "BEQ" || opcode == "beq") {
            int rs1 = stoi(pInstr.operands[0].substr(1));
            int rs2 = stoi(pInstr.operands[1].substr(1));
            string label = pInstr.operands[2];
            if (registers[rs1] == registers[rs2] && labels.count(label)) {
                cout << "BEQ: X" << rs1 << " == X" << rs2 << ", Jumping to " << label << "\n";
                pc = labels[label]; // Update core's PC
                return true;
            }
        }

        else if (opcode == "BLE" || opcode == "ble") {
            int rs1 = stoi(pInstr.operands[0].substr(1));
            int rs2 = stoi(pInstr.operands[1].substr(1));
            string label = pInstr.operands[2];
            if (registers[rs1] <= registers[rs2] && labels.count(label)) {
                cout << "BLE: X" << rs1 << " <= X" << rs2 << ", Jumping to " << label << "\n";
                pc = labels[label]; // Update core's PC
                return true;
            }
        }

        else if (opcode == "J" || opcode == "j") {
            string label = pInstr.operands[0];
            if (labels.count(label)) {
                cout << "J: Unconditional jump to " << label << "\n";
                pc = labels[label]; // Update core's PC
                return true;
            }
        }

        else if (opcode == "BNE" || opcode == "bne") {
            int rs1 = stoi(pInstr.operands[0].substr(1));
            int rs2 = stoi(pInstr.operands[1].substr(1));
            string label = pInstr.operands[2];
            if (registers[rs1] != registers[rs2] && labels.count(label)) {
                cout << "BNE: X" << rs1 << " != X" << rs2 << ", Jumping to " << label << "\n";
                pc = labels[label]; // Update core's PC
                return true;
            }
        }
        else if (opcode == "ECALL" || opcode == "ecall") {
            cout << "System call executed\n";
        }

        else if (opcode == "LA" || opcode == "la") {
            int rd = stoi(pInstr.operands[0].substr(1));
            string label = pInstr.operands[1];

            if (data.find(label) != data.end()) {
                registers[rd] = data[label];
                cout << "Loaded Address " << label << " -> X" << rd << " = " << registers[rd] << "\n";
            } else {
                cerr << "Error: Label " << label << " not found in .data\n";
            }
        }

        else if (opcode == "LI" || opcode == "li") {
            int rd = stoi(pInstr.operands[0].substr(1));
            int imm = stoi(pInstr.operands[1]);
            registers[rd] = imm;
            cout << "LI: Loaded immediate " << imm << " into X" << rd << "\n";
        }

        else if (opcode == "SW" || opcode == "sw") {
            int rs = stoi(pInstr.operands[0].substr(1));
            string offset_reg = pInstr.operands[1];
            int offset;

            size_t open_bracket = offset_reg.find('(');
            size_t close_bracket = offset_reg.find(')');

            if (open_bracket != string::npos && close_bracket != string::npos) {
                string offset_str = offset_reg.substr(0, open_bracket);
                string base_str = offset_reg.substr(open_bracket + 1, close_bracket - open_bracket - 1);

                offset = stoi(offset_str);
                int base = stoi(base_str.substr(1));

                int mem_address = registers[base] + offset;

                if ((mem_address >= 0) && (mem_address < mem.size() * 4) && (mem_address % 4 == 0)) {
                    mem[mem_address / 4] = registers[rs];
                    cout << "Stored " << registers[rs] << " to Mem[" << mem_address << "]\n";
                } else {
                    cerr << "Error: Memory access out of bounds or unaligned at " << mem_address << "\n";
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

                int mem_address = registers[base] + offset;

                if ((mem_address >= 0) && (mem_address < mem.size() * 4) && (mem_address % 4 == 0)) {
                    registers[rd] = mem[mem_address / 4];
                    cout << "Loaded " << registers[rd] << " from Mem[" << (mem_address / 4) << "]\n";
                } else {
                    cerr << "Error: Memory access out of bounds or unaligned at " << mem_address << "\n";
                }
            } else {
                cerr << "Error parsing LW instruction: " << pInstr.instruction << endl;
            }
        }

        return true;
    }

    PipelineInstruction decode(const PipelineInstruction& pInstr) {
        PipelineInstruction decodedInstr = pInstr;
        cout << "Core " << coreid << " decoding: " << decodedInstr.instruction << " (PC = " << decodedInstr.pc << ")\n";
        return decodedInstr;
    }

    PipelineInstruction memory(const PipelineInstruction& pInstr, vector<int>& mem) {
        PipelineInstruction memInstr = pInstr;
        cout << "Core " << coreid << " memory: " << memInstr.instruction << " (PC = " << memInstr.pc << ")\n";
        return memInstr;
    }

    PipelineInstruction writeback(const PipelineInstruction& pInstr) {
        PipelineInstruction wbInstr = pInstr;
        cout << "Core " << coreid << " writeback: " << wbInstr.instruction << " (PC = " << wbInstr.pc << ")\n";
        return wbInstr;
    }

    PipelineInstruction fetch(int pc, vector<string>& program) {
        PipelineInstruction fetchedInstr;

        if (pc >= program.size()) {
            return fetchedInstr; // Invalid instruction
        }

        fetchedInstr.valid = true;
        fetchedInstr.pc = pc;
        fetchedInstr.instruction = program[pc];

        istringstream iss(program[pc]);
        iss >> fetchedInstr.opcode;

        string operand;
        while (iss >> operand) {
            fetchedInstr.operands.push_back(operand);
        }

        cout << "Core " << coreid << " fetch: " << program[pc] << " (PC = " << pc << ")\n";
        return fetchedInstr;
    }
};

class Simulator {
public:
    vector<int> memory;
    int clock;
    vector<Cores> cores;
    vector<string> program;
    unordered_map<string, int> labels;
    unordered_map<string, int> data;

    Simulator() : memory(1024, 0), clock(0), cores(4, Cores(0)) {
        for (int i = 0; i < 4; ++i) {
            cores[i] = Cores(i);
        }
    }

    void initializeMemory() {
        for (int i = 0; i < memory.size(); ++i) {
            memory[i] = 0;
        }
    }

    void parseDataSection(const vector<string>& pgm) {
        int memIndex = 0;
        bool inDataSection = false;

        for (const string& line : pgm) {
            istringstream iss(line);
            string first_word;
            iss >> first_word;

            if (first_word == ".data") {
                inDataSection = true;
                continue;
            }

            if (first_word == ".text") {
                inDataSection = false;
                continue;
            }

            if (inDataSection && first_word.back() == ':') {
                string label = first_word.substr(0, first_word.size() - 1);
                data[label] = memIndex * 4;

                string type;
                iss >> type;

                if (type == ".word") {
                    string numStr;
                    while (iss >> numStr) {
                        int value;

                        // Check if number starts with "0x" for hexadecimal
                        if (numStr.find("0x") == 0 || numStr.find("0X") == 0) {
                            value = stoi(numStr, nullptr, 16);  // Convert from HEX
                        } else {
                            value = stoi(numStr, nullptr, 10);  // Convert from DECIMAL
                        }

                        memory[memIndex] = value;
                        cout << "Stored " << value << " at Mem[" << memIndex * 4 << "] " << endl;
                        memIndex++;
                    }
                }
            }
        }
    }

    void storingLabels() {
        for (int i = 0; i < program.size(); ++i) {
            istringstream iss(program[i]);
            string first_word;
            iss >> first_word;
            if (!first_word.empty() && first_word.back() == ':') {
                labels[first_word.substr(0, first_word.size() - 1)] = i;
            }
        }
    }

    void run() {
        storingLabels();
        parseDataSection(program);

        vector<PipelineInstruction> if_id(cores.size());
        vector<PipelineInstruction> id_ex(cores.size());
        vector<PipelineInstruction> ex_mem(cores.size());
        vector<PipelineInstruction> mem_wb(cores.size());

        int fetch_pc = 0;
        bool program_complete = false;
        bool firstInstructionFetched = false; // Track if the first instruction has been fetched

        while (!program_complete) {
            bool all_cores_idle = true;

            // Fetch Stage
            PipelineInstruction fetchedInstruction;
            if (fetch_pc < program.size()) {
                fetchedInstruction = cores[0].fetch(fetch_pc, program);
                if (clock == 0){
                    cout << "Clock cycle: " << clock+1 << endl;
                    cout<<"-------------------------------------"<<endl;
                }
                else{
                    cout << "Clock cycle: " << clock << endl;
                    cout<<"-------------------------------------"<<endl;
                }
                
                if (fetchedInstruction.valid && !firstInstructionFetched) { // First valid fetch
                    clock++;
                    
                    firstInstructionFetched = true;
                }
                
                fetch_pc++;
            } else {
                fetchedInstruction.valid = false; // No more instructions to fetch
            }

            // Pipeline stages for all cores
            for (int i = 0; i < cores.size(); ++i) {
                // Copy the shared instruction to every core
                if_id[i] = fetchedInstruction;

                // Writeback
                if (mem_wb[i].valid) {
                    mem_wb[i] = cores[i].writeback(mem_wb[i]);
                    mem_wb[i].valid = false;
                }
                // Memory
                if (ex_mem[i].valid) {
                    mem_wb[i] = cores[i].memory(ex_mem[i], memory);
                    ex_mem[i].valid = false;
                }
                // Execute
                if (id_ex[i].valid) {
                    ex_mem[i] = id_ex[i];
                    cores[i].execute(ex_mem[i], memory, labels, data);
                    id_ex[i].valid = false;
                }
                // Decode
                if (if_id[i].valid) {
                    id_ex[i] = cores[i].decode(if_id[i]);
                    if_id[i].valid = false;
                }
              // clock++;

            }
            if(firstInstructionFetched == true){
                 clock++;
                cout << "Clock cycle: " << clock << endl;
                cout<<"----------------------------------"<<endl;
            }
            

            // Check if all cores are done
            program_complete = (fetch_pc >= program.size() &&
                !if_id[0].valid && !id_ex[0].valid && !ex_mem[0].valid && !mem_wb[0].valid &&
                !if_id[1].valid && !id_ex[1].valid && !ex_mem[1].valid && !mem_wb[1].valid &&
                !if_id[2].valid && !id_ex[2].valid && !ex_mem[2].valid && !mem_wb[2].valid &&
                !if_id[3].valid && !id_ex[3].valid && !ex_mem[3].valid && !mem_wb[3].valid);
           // program_complete = true;
        }

        cout << "Simulation finished after " << clock << " clock cycles." << endl;
    }

    void display() {
        for (int i = 0; i < 4; ++i) {
            cout << "Core " << i << " Registers: ";
            for (int j = 0; j < 32; ++j) {
                cout << setw(3) << cores[i].registers[j] << " ";
            }
            cout << endl;
        }
        cout << "Memory: ";
        for (int i = 0; i < 16; ++i) {
            cout << setw(3) << memory[i] << " ";
        }
        cout << "\n";
        cout << "Number of clock cycles: " << clock << endl;
    }
};

int main() {
    vector<string> program;

    ifstream file("bubbleSort.s");
    if (!file.is_open()) {
        cerr << "Failed to open file: program.s" << endl;
        return 1;
    }

    string line;
    while (getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != string::npos) {
            line = line.substr(0, comment_pos);  //Remove comment part
        }
        if (!line.empty()) {  // Skip empty lines
            program.push_back(line);
        }
    }
    file.close();

    Simulator sim;
    sim.program = program;
    sim.parseDataSection(program);
    sim.initializeMemory();

    for (int i = 0; i < 4; i++) {
        sim.cores[i].registers[1] = 1;
        sim.cores[i].registers[2] = 2;
        sim.cores[i].registers[3] = 3;
    }

    sim.run();
    sim.display();

    return 0;
}