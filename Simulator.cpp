#include "Simulator.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace std;

Simulator::Simulator() : memory(1024, 0), clock(0), cores(4, Cores(0)),
    stallCount(0), instructionsExecuted(0), forwardingEnabled(true) {
    for (int i = 0; i < 4; ++i)
        cores[i] = Cores(i);
}

void Simulator::initializeMemory() {
    for (int i = 0; i < static_cast<int>(memory.size()); ++i)
        memory[i] = i;
}

void Simulator::parseDataSection(const vector<string>& pgm) {
    int memIndex = 0;
    bool inDataSection = false;
    for (const string &line : pgm) {
        istringstream iss(line);
        string first;
        iss >> first;
        if (first == ".data") { 
            inDataSection = true;
            continue;
        }
        if (first == ".text") { 
            inDataSection = false;
            continue;
        }
        if (inDataSection && first.back() == ':') {
            string label = first.substr(0, first.size() - 1);
            data[label] = memIndex * 4;
            string type;
            iss >> type;
            if (type == ".word") {
                string num;
                while (iss >> num) {
                    int value = stoi(num, nullptr, (num.find("0x") == 0) ? 16 : 10);
                    memory[memIndex] = value;
                    cout << "Stored " << value << " at Mem[" << memIndex * 4 << "]\n";
                    memIndex++;
                }
            }
        }
    }
}

void Simulator::storingLabels() {
    for (int i = 0; i < static_cast<int>(program.size()); ++i) {
        istringstream iss(program[i]);
        string first;
        iss >> first;
        if (!first.empty() && first.back() == ':')
            labels[first.substr(0, first.size() - 1)] = i;
    }
}

void Simulator::run() {
    storingLabels();
    parseDataSection(program);

    // Pipeline registers for each core.
    std::vector<PipelineInstruction> if_id(cores.size());
    std::vector<PipelineInstruction> id_ex(cores.size());
    std::vector<PipelineInstruction> ex_mem(cores.size());
    std::vector<PipelineInstruction> mem_wb(cores.size());
    // Save previous cycle's ID/EX for hazard detection.
    std::vector<PipelineInstruction> prev_id_ex(cores.size());

    int fetch_pc = 0;
    bool program_complete = false;
    // Stall flag per core.
    std::vector<bool> stall(cores.size(), false);

    // Initialize pipeline registers.
    for (int i = 0; i < static_cast<int>(cores.size()); ++i) {
        if_id[i].valid = false;
        id_ex[i].valid = false;
        ex_mem[i].valid = false;
        mem_wb[i].valid = false;
        prev_id_ex[i].valid = false;
    }

    while (!program_complete) {
        cout << "\nClock cycle: " << clock << "\n";
        cout << "-------------------------------------\n";

        // Writeback Stage.
        for (int i = 0; i < static_cast<int>(cores.size()); ++i) {
            if (mem_wb[i].valid && !mem_wb[i].stalled) {
                mem_wb[i] = cores[i].writeback(mem_wb[i]);
                mem_wb[i].valid = false;
                instructionsExecuted++;
            }
        }

        // Memory Stage.
        for (int i = 0; i < static_cast<int>(cores.size()); ++i) {
            if (ex_mem[i].valid && !ex_mem[i].stalled) {
                mem_wb[i] = ex_mem[i];
                cores[i].memory(mem_wb[i], memory);
                ex_mem[i].valid = false;
            }
        }

        // Hazard Detection in Execute Stage.
        for (int i = 0; i < static_cast<int>(cores.size()); ++i) {
            if (!forwardingEnabled && id_ex[i].valid && prev_id_ex[i].valid) {
                if (prev_id_ex[i].rd != -1) {
                    if ((id_ex[i].rs1 != -1 && id_ex[i].rs1 == prev_id_ex[i].rd) ||
                        (id_ex[i].rs2 != -1 && id_ex[i].rs2 == prev_id_ex[i].rd)) {
                        stall[i] = true;
                        stallCount++;
                        cout << "Stalling execute stage on core " << i 
                             << " due to RAW hazard with previous ID/EX (X" 
                             << prev_id_ex[i].rd << ")\n";
                        id_ex[i].stalled = true;
                    } else {
                        id_ex[i].stalled = false;
                        stall[i] = false;
                    }
                }
            }
        }
        // Save current ID/EX for next cycle.
        prev_id_ex = id_ex;

        // Execute Stage with Latency.
        for (int i = 0; i < static_cast<int>(cores.size()); ++i) {
            if (id_ex[i].valid && !id_ex[i].stalled) {
                if (id_ex[i].execCyclesRemaining > 1) {
                    id_ex[i].execCyclesRemaining--;
                    cout << "Core " << i << " executing latency, remaining cycles: " 
                         << id_ex[i].execCyclesRemaining << "\n";
                } else { // Execute when latency reaches 1.
                    PipelineInstruction inst = id_ex[i];
                    inst.op1 = cores[i].registers[inst.rs1];
                    if (inst.rs2 != -1)
                        inst.op2 = cores[i].registers[inst.rs2];
                    ex_mem[i] = inst;
                    ex_mem[i].aluResult = cores[i].execute(ex_mem[i], memory, labels, data);
                    id_ex[i].valid = false;
                }
            }
        }

        // Decode Stage.
        for (int i = 0; i < static_cast<int>(cores.size()); ++i) {
            if (if_id[i].valid && !id_ex[i].valid) {
                PipelineInstruction decoded = cores[i].decode(if_id[i]);
                // Set execution latency based on opcode.
                std::string op = decoded.opcode;
                std::transform(op.begin(), op.end(), op.begin(), ::toupper);
                if (latencies.find(op) != latencies.end()) {
                    decoded.execCyclesRemaining = latencies[op];
                    cout << "Setting latency for " << decoded.opcode << " to " 
                         << decoded.execCyclesRemaining << " cycles\n";
                } else {
                    decoded.execCyclesRemaining = 1; // Default latency.
                }
                id_ex[i] = decoded;
                if_id[i].valid = false;
            }
        }

        // Fetch Stage.
        PipelineInstruction fetched;
        if (fetch_pc < static_cast<int>(program.size())) {
            fetched = cores[0].fetch(fetch_pc, program);
            fetch_pc++;
        } else {
            fetched.valid = false;
        }
        for (int i = 0; i < static_cast<int>(cores.size()); ++i) {
            if (fetched.valid)
                if_id[i] = fetched;
        }

        clock++;
        program_complete = (fetch_pc >= static_cast<int>(program.size()) &&
                            !if_id[0].valid && !id_ex[0].valid &&
                            !ex_mem[0].valid && !mem_wb[0].valid);
    }
    cout << "\nSimulation finished after " << clock << " cycles.\n";
}

void Simulator::display() {
    for (int i = 0; i < 4; ++i) {
        cout << "Core " << i << " Registers: ";
        for (int j = 0; j < 32; ++j)
            cout << std::setw(3) << cores[i].registers[j] << " ";
        cout << "\n";
    }
    cout << "Memory: ";
    for (int i = 0; i < 16; ++i) {
        cout << std::setw(3) << memory[i] << " ";
    }
    cout << "\n";
    cout << "Total clock cycles: " << clock << "\n";
    cout << "Total instructions executed: " << instructionsExecuted << "\n";
    cout << "Total stalls: " << stallCount << "\n";
    double ipc = (clock > 0) ? static_cast<double>(instructionsExecuted) / clock : 0;
    cout << "IPC: " << ipc << "\n";
}
