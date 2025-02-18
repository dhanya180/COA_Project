#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include<unordered_map>
using namespace std;
class Cores {
public:
    vector<int> registers;
    int pc;
    int coreid;

    Cores(int cid) : registers(32, 0), pc(0), coreid(cid) {}

    bool execute(const vector<string>& pgm, vector<int>& mem ,unordered_map<string, int>& labels) {
        if (pc >= pgm.size()) return false;
        
        istringstream iss(pgm[pc]);
        string opcode;
        iss >> opcode ;
        if(opcode.back() == ':'){
            pc++;
            return true;
        }
        cout << "Core " << coreid << " executing: " << pgm[pc] << " (PC = " << pc << ")\n";


        if (opcode == "ADD") {
        string rd_str, rs1_str, rs2_str;
        iss>> rd_str >> rs1_str >> rs2_str;
        int rd = stoi(rd_str.substr(1));
        int rs1 = stoi(rs1_str.substr(1));
        int rs2 = stoi(rs2_str.substr(1));
        registers[rd] = registers[rs1] + registers[rs2];
        } 
        else if (opcode == "SUB") {
        string rd_str, rs1_str, rs2_str;
        iss>> rd_str >> rs1_str >> rs2_str;
        int rd = stoi(rd_str.substr(1));
        int rs1 = stoi(rs1_str.substr(1));
        int rs2 = stoi(rs2_str.substr(1));
        registers[rd] = registers[rs1] - registers[rs2];
        }

        else if (opcode == "BNE") {
            std::string rs1_str, rs2_str, label;
            iss >> rs1_str >> rs2_str >> label;
            int rs1 = std::stoi(rs1_str.substr(1));
            int rs2 = std::stoi(rs2_str.substr(1));
            if (registers[rs1] != registers[rs2] && labels.count(label)) {
                std::cout << "BNE: X" << rs1 << " != X" << rs2 << ", Jumping to " << label << "\n";
                pc = labels[label];
                return true;    
            }
        }
        else if (opcode == "JAL") {
            std::string rd_str, label;
            iss >> rd_str >> label;
            if (labels.count(label)) {
                std::cout << "JAL: Jumping to " << label << "\n";
                pc = labels[label];
                return true;
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
    unordered_map<string,int> labels;

    Simulator() : memory(4096 / 4, 0), clock(0), cores(4, Cores(0)) {
        for (int i = 0; i < 4; ++i) {
            cores[i] = Cores(i);
        }
    }



    void storingLabels(){
        for (int i = 0; i < program.size(); ++i) {
            std::istringstream iss(program[i]);
            std::string first_word;
            iss >> first_word;
            if (first_word.back() == ':') {
                labels[first_word.substr(0, first_word.size() - 1)] = i;
            }
        }

    }
    void run() {
        storingLabels();
        while (clock < program.size()) {
            for (int i = 0; i < 4; ++i) {
                cores[i].execute(program, memory,labels);
            }
            clock++;
        }
    }
};



int main() {
    Simulator sim;
    sim.program = {
        "LABELx:",
        "ADD X0 X0 X1",   // X0 = X0 + X1
        "SUB X0 X0 X1",   // X0 = X0 - X1
        "BNE X0 X2 LABELy",  // Branch to LABELy if X0 != X2
        //"SW X0 0(X3)",    // Store word from X0 into memory at address X3 + 0
        "LABELy:",
        "SUB X0 X0 X2",   // X0 = X0 - X2
        "JAL X0 LABELx"   // Jump to LABELx and store return address in X0
    };

    for(int i=0;i<4;i++){
        sim.cores[i].registers[1] = 1;
        sim.cores[i].registers[2] = 2;
        sim.cores[i].registers[3]= 3;
    }
    
    sim.run();

    for (int i = 0; i < 4; ++i) {
        cout << "Core " << i << " Registers: ";
        for (int j = 0; j < 32; ++j) {
            cout << setw(3) << sim.cores[i].registers[j] << " ";
        }
       cout << endl;
    }
    cout << "Number of clock cycles: " << sim.clock << endl;

    return 0;
}

