#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
using namespace std;
class Cores {
public:
    vector<int> registers;
    int pc;
    int coreid;

    Cores(int cid) : registers(32, 0), pc(0), coreid(cid) {}

    void execute(const vector<string>& pgm, vector<int>& mem) {
        if (pc >= pgm.size()) return;
        
        istringstream iss(pgm[pc]);
        string opcode, rd_str, rs1_str, rs2_str;
        iss >> opcode >> rd_str >> rs1_str >> rs2_str;

        int rd = stoi(rd_str.substr(1));
        int rs1 = stoi(rs1_str.substr(1));
        int rs2 = stoi(rs2_str.substr(1));

        if (opcode == "ADD") {
            registers[rd] = registers[rs1] + registers[rs2];
        } 
        else if (opcode == "SUB") {
           registers[rd] = registers[rs1] - registers[rs2];
        }
        pc++;
    }
};

class Simulator {
public:
    vector<int> memory;
    int clock;
    vector<Cores> cores;
    vector<string> program;

    Simulator() : memory(4096 / 4, 0), clock(0), cores(4, Cores(0)) {
        for (int i = 0; i < 4; ++i) {
            cores[i] = Cores(i);
        }
    }

    void run() {
        while (clock < program.size()) {
            for (int i = 0; i < 4; ++i) {
                cores[i].execute(program, memory);
            }
            clock++;
        }
    }
};

int main() {
    Simulator sim;
    sim.program = {"ADD X4 X5 X6", "SUB X7 X8 X9"};
    for(int i=0;i<4;i++){
        sim.cores[i].registers[5] = 3;
        sim.cores[i].registers[8] = 12;
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