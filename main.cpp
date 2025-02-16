#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

class Cores {
public:
    std::vector<int> registers;
    int pc;
    int coreid;

    Cores(int cid) : registers(32, 0), pc(0), coreid(cid) {}

    void execute(const std::vector<std::string>& pgm, std::vector<int>& mem) {
        if (pc >= pgm.size()) return;
        
        std::istringstream iss(pgm[pc]);
        std::string opcode, rd_str, rs1_str, rs2_str;
        iss >> opcode >> rd_str >> rs1_str >> rs2_str;

        int rd = std::stoi(rd_str.substr(1));
        int rs1 = std::stoi(rs1_str.substr(1));
        int rs2 = std::stoi(rs2_str.substr(1));

        if (opcode == "ADD") {
            registers[rd] = registers[rs1] + registers[rs2];
        } else if (opcode == "LD") {
            // TODO: Implement memory instructions
        }
        pc++;
    }
};

class Simulator {
public:
    std::vector<int> memory;
    int clock;
    std::vector<Cores> cores;
    std::vector<std::string> program;

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
    sim.program = {"ADD X4 X5 X6", "ADD X7 X8 X9"};
    sim.cores[0].registers[5] = 3;
    sim.cores[0].registers[8] = 2;
    sim.run();

    for (int i = 0; i < 4; ++i) {
        std::cout << "Core " << i << " Registers: ";
        for (int j = 0; j < 32; ++j) {
            std::cout << std::setw(3) << sim.cores[i].registers[j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "Number of clock cycles: " << sim.clock << std::endl;

    return 0;
}