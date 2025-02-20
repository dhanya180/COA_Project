#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <unordered_map>

using namespace std;

class Cores {
public:
    vector<int> registers;
    int pc;
    int coreid;

    Cores(int cid) : registers(32, 0), pc(0), coreid(cid) {}

    bool execute(const vector<string>& pgm, vector<int>& mem, unordered_map<string, int>& labels , unordered_map<string, int>& data) {
        if (pc >= pgm.size()) return false;

        istringstream iss(pgm[pc]);
        string opcode;
        iss >> opcode;
        if (opcode.empty() || opcode[0] == '#') {  // Ignore empty lines and comments
            pc++;
            return true;
        }

        if (opcode.back() == ':') {  // Skip labels
            pc++;
            return true;
        }

        cout << "Core " << coreid << " executing: " << pgm[pc] << " (PC = " << pc << ")\n";

        if (opcode == "ADD" || opcode == "add") {
            string rd_str, rs1_str, rs2_str;
            iss >> rd_str >> rs1_str >> rs2_str;
            int rd = stoi(rd_str.substr(1));
            int rs1 = stoi(rs1_str.substr(1));
            int rs2 = stoi(rs2_str.substr(1));
            registers[rd] = registers[rs1] + registers[rs2];
            cout << "Updated X" << rd << " = " << registers[rd] << "\n";
        }

        else if (opcode == "ADDI" || opcode == "addi") {
            string rd_str, rs1_str;
            int imm;
            iss >> rd_str >> rs1_str >> imm;
            int rd = stoi(rd_str.substr(1));
            int rs1 = stoi(rs1_str.substr(1));
            registers[rd] = registers[rs1] + imm;
            cout << "Updated X" << rd << " = " << registers[rd] << "\n";
        }

        else if (opcode == "SUB" || opcode == "sub") {
            string rd_str, rs1_str, rs2_str;
            iss >> rd_str >> rs1_str >> rs2_str;
            int rd = stoi(rd_str.substr(1));
            int rs1 = stoi(rs1_str.substr(1));
            int rs2 = stoi(rs2_str.substr(1));
            registers[rd] = registers[rs1] - registers[rs2];
            cout << "Updated X" << rd << " = " << registers[rd] << "\n";
        }

        else if (opcode == "JAL" || opcode == "jal") {
            string rd_str, label;
            iss >> rd_str >> label;
            if (labels.count(label)) {
                cout << "JAL: Jumping to " << label << "\n";
                registers[31] = pc + 1;
                pc = labels[label];
                return true;
            }
        }

        else if (opcode == "BEQ" || opcode == "beq") {
            string rs1_str, rs2_str, label;
            iss >> rs1_str >> rs2_str >> label;
            int rs1 = stoi(rs1_str.substr(1));
            int rs2 = stoi(rs2_str.substr(1));
            if (registers[rs1] == registers[rs2] && labels.count(label)) {
                pc = labels[label];
                return true;
            }
        }
        
        else if (opcode == "BLE" || opcode == "ble") {
            string rs1_str, rs2_str, label;
            iss >> rs1_str >> rs2_str >> label;
            int rs1 = stoi(rs1_str.substr(1));
            int rs2 = stoi(rs2_str.substr(1));
            if (registers[rs1] <= registers[rs2] && labels.count(label)) {
                pc = labels[label];
                return true;
            }
        }
        
        else if (opcode == "J" || opcode == "j") {
            string label;
            iss >> label;
            if (labels.count(label)) {
                pc = labels[label];
                return true;
            }
        }

        else if (opcode == "BNE" || opcode == "bne") {
            string rs1_str, rs2_str, label;
            iss >> rs1_str >> rs2_str >> label;
            int rs1 = stoi(rs1_str.substr(1));
            int rs2 = stoi(rs2_str.substr(1));
            if (registers[rs1] != registers[rs2] && labels.count(label)) {
                cout << "BNE: X" << rs1 << " != X" << rs2 << ", Jumping to " << label << "\n";
                pc = labels[label];
                return true;
            }
        }

        else if (opcode == "ECALL" || opcode == "ecall") {
            cout << "System call executed\n";
        }
        
        else if (opcode == "LA" || opcode == "la") {
            string rd_str, label;
            iss >> rd_str >> label;
            int rd = stoi(rd_str.substr(1));
        
            if (data.find(label) != data.end()) {
                registers[rd] = data[label];  
                cout << "Loaded Address " << label << " -> X" << rd << " = " << registers[rd] << "\n";
            } else {
                cerr << "Error: Label " << label << " not found in .data\n";
            }
        }
        
        
        else if (opcode == "LI" || opcode == "li") {
            string rd_str;
            int imm;
            iss >> rd_str >> imm;
            int rd = stoi(rd_str.substr(1));
            registers[rd] = imm;
        }
        
        // else if (opcode == "LW" || opcode == "lw") {
        //     string rd_str, offset_reg;
        //     int offset;
        //     iss >> rd_str >> offset >> offset_reg;
        //     int rd = stoi(rd_str.substr(1));
        //     int base = stoi(offset_reg.substr(1));
        //     int mem_address = registers[base] + offset;
        //     registers[rd] = mem[mem_address / 4];
        //     cout << "Loaded " << registers[rd] << " from Mem[" << mem_address << "]\n";
        // }
        //  else if (opcode == "LW" || opcode == "lw") {
        //     string rd_str, offset_reg;
        //     int offset;
            
        //     // Read the next two parts
        //     iss >> rd_str >> offset_reg;
            
        //     // Extract offset and register from "0(x3)" format
        //     size_t open_bracket = offset_reg.find('(');
        //     size_t close_bracket = offset_reg.find(')');
        
        //     if (open_bracket != string::npos && close_bracket != string::npos) {
        //         // Extract offset and base register separately
        //         string offset_str = offset_reg.substr(0, open_bracket); // Extract "0"
        //         string base_str = offset_reg.substr(open_bracket + 1, close_bracket - open_bracket - 1); // Extract "x3"
        
        //         // Convert strings to integers safely
        //         offset = stoi(offset_str);
        //         int base = stoi(base_str.substr(1)); // Remove 'x' and convert
                
        //         int rd = stoi(rd_str.substr(1)); // Remove 'x' from rd
                
        //         // Perform memory access
        //         int mem_address = registers[base] + offset;
        //         registers[rd] = mem[mem_address / 4];  // Read from memory
        //         cout << "Loaded " << registers[rd] << " from Mem[" << mem_address << "]\n";
        //     } else {
        //         cerr << "Error parsing LW instruction: " << pgm[pc] << endl;
        //     }
        // }
        // else if (opcode == "LW" || opcode == "lw") {  // Load Word
        //     string rd_str, offset_reg;
        //     int offset;
        //     iss >> rd_str >> offset >> offset_reg;
        //     int rd = stoi(rd_str.substr(1));
        //     int base = stoi(offset_reg.substr(1));
        //     int mem_address = registers[base] + offset;

        //     if (mem_address / 4 < mem.size()) {
        //         registers[rd] = mem[mem_address / 4];
        //         cout << "Loaded " << registers[rd] << " from Mem[" << mem_address << "]\n";
        //     } else {
        //         cerr << "Error: Memory access out of bounds at " << mem_address << "\n";
        //     }
        // }
        

        // else if (opcode == "SW" || opcode == "sw") {
        //     string rs_str, offset_reg;
        //     int offset;
        //     iss >> rs_str >> offset >> offset_reg;
        //     int rs = stoi(rs_str.substr(1));
        //     int base = stoi(offset_reg.substr(1));
        //     int mem_address = registers[base] + offset;
        //     if (mem_address / 4 < mem.size()) {
        //         mem[mem_address / 4] = registers[rs];
        //         cout << "Stored " << registers[rs] << " to Mem[" << mem_address << "]\n";
        //     } else {
        //         cerr << "Error: Memory access out of bounds at " << mem_address << "\n";
        //     }
        // }
        else if (opcode == "SW" || opcode == "sw") {
            string rs_str, offset_reg;
            int offset;
            iss >> rs_str >> offset_reg;
    
            size_t open_bracket = offset_reg.find('(');
            size_t close_bracket = offset_reg.find(')');
    
            if (open_bracket != string::npos && close_bracket != string::npos) {
                string offset_str = offset_reg.substr(0, open_bracket);
                string base_str = offset_reg.substr(open_bracket + 1, close_bracket - open_bracket - 1);
    
                offset = stoi(offset_str);
                int base = stoi(base_str.substr(1));
                int rs = stoi(rs_str.substr(1));
    
                int mem_address = registers[base] + offset;
    
                if ((mem_address >= 0) && (mem_address < mem.size() * 4) && (mem_address % 4 == 0)) { 
                    mem[mem_address / 4] = registers[rs];
                    cout << "Stored " << registers[rs] << " to Mem[" << mem_address << "]\n";
                } else {
                    cerr << "Error: Memory access out of bounds or unaligned at " << mem_address << "\n";
                }
            } else {
                cerr << "Error parsing SW instruction: " << pgm[pc] << endl;
            }
        }
    
        else if (opcode == "LW" || opcode == "lw") {
            string rd_str, offset_reg;
            int offset;
    
            iss >> rd_str >> offset_reg;

            size_t open_bracket = offset_reg.find('(');
            size_t close_bracket = offset_reg.find(')');
    
            if (open_bracket != string::npos && close_bracket != string::npos) {
                string offset_str = offset_reg.substr(0, open_bracket); 
                string base_str = offset_reg.substr(open_bracket + 1, close_bracket - open_bracket - 1); 
    
               
                offset = stoi(offset_str);
                int base = stoi(base_str.substr(1)); 
    
                int rd = stoi(rd_str.substr(1)); 
                 int mem_address = registers[base] + offset;
    
                if ((mem_address >= 0) && (mem_address < mem.size() * 4) && (mem_address % 4 == 0)) { 
                    registers[rd] = mem[mem_address / 4];
                    cout << "Loaded " << registers[rd] << " from Mem[" << mem_address << "]\n";
                } else {
                    cerr << "Error: Memory access out of bounds or unaligned at " << mem_address << "\n";
                }
            } else {
                cerr << "Error parsing LW instruction: " << pgm[pc] << endl;
            }
        }
    

        pc++;
        return true;
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
            memory[i] = i * 2;
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
                    int value;
                    while (iss >> hex >> value) {  
                        memory[memIndex++] = value;
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

    // void run() {
    //     storingLabels();
    //     parseDataSection(program);
    //     while (clock < program.size()) {
    //         for (int i = 0; i < 4; ++i) {
    //             cores[i].execute(program, memory, labels,data);
    //         }
    //         clock++;
    //     }
    // }
    void run() {
        storingLabels();
        parseDataSection(program);
    
        vector<bool> core_active(cores.size(), true); 
    
        while (true) {
            bool all_cores_idle = true; 
    
            for (int i = 0; i < cores.size(); ++i) {
                if (core_active[i]) {
                    all_cores_idle = false; 
    
                    if (cores[i].pc < program.size()) { 
                        core_active[i] = cores[i].execute(program, memory, labels, data);
                    } else {
                        core_active[i] = false; 
                    }
                }
            }
    
            if (all_cores_idle) {
                break; 
            }
    
            clock++; 
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
