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

    bool execute(const vector<string>& pgm, vector<int>& mem, unordered_map<string, int>& labels) {
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

        else if (opcode == "LW" || opcode == "lw") {
            string rd_str, offset_reg;
            int offset;
            iss >> rd_str >> offset >> offset_reg;
            int rd = stoi(rd_str.substr(1));
            int base = stoi(offset_reg.substr(1));
            int mem_address = registers[base] + offset;
            registers[rd] = mem[mem_address / 4];
            cout << "Loaded " << registers[rd] << " from Mem[" << mem_address << "]\n";
        }

        else if (opcode == "SW" || opcode == "sw") {
            string rs_str, offset_reg;
            int offset;
            iss >> rs_str >> offset >> offset_reg;
            int rs = stoi(rs_str.substr(1));
            int base = stoi(offset_reg.substr(1));
            int mem_address = registers[base] + offset;
            mem[mem_address / 4] = registers[rs];
            cout << "Stored " << registers[rs] << " to Mem[" << mem_address << "]\n";
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
        while (clock < program.size()) {
            for (int i = 0; i < 4; ++i) {
                cores[i].execute(program, memory, labels);
            }
            clock++;
        }
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

    ifstream file("program.s");
    if (!file.is_open()) {
        cerr << "Failed to open file: program.s" << endl;
        return 1;
    }

    string line;
    while (getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != string::npos) {
            line = line.substr(0, comment_pos);  // Remove comment part
        }
        if (!line.empty()) {  // Skip empty lines
            program.push_back(line);
        }
    }
    file.close();

    Simulator sim;
    sim.program = program;
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
