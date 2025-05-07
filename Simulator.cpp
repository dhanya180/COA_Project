#include "Simulator.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace std;

Simulator::Simulator() : memory(1024, 0), clock(0), cores(4, Cores(0)),
stallCount(0), controlHazardCount(0), instructionsExecuted(0), forwardingEnabled(true) {
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
        for(const string &line : pgm) {
            istringstream iss(line);
            string first;
            iss >> first;
            
            // Skip empty lines or comments
            if (first.empty() || first[0] == '#' || first[0] == '/') {
                continue;
            }
            
            if(first == ".data") { 
                inDataSection = true; 
                continue; 
            }
            if(first == ".text") { 
                inDataSection = false; 
                continue; 
            }
            
            if(inDataSection && first.back() == ':') {
                string label = first.substr(0, first.size()-1);
                data[label] = memIndex*4;
                string type;
                iss >> type;
                if(type == ".word") {
                    string num;
                    while(iss >> num) {
                        // Skip comments
                        if (num[0] == '#' || num.substr(0, 2) == "//") {
                            break;
                        }
                        
                        // Remove commas
                        if (!num.empty() && num.back() == ',') {
                            num.pop_back();
                        }
                        
                        try {
                            int value = stoi(num, nullptr, (num.find("0x") == 0) ? 16 : 10);
                            memory[memIndex] = value;
                            cout << "Stored " << value << " at Mem[" << memIndex*4 << "]\n";
                            memIndex++;
                        } catch (exception& e) {
                            cerr << "ERROR: Failed to parse memory value: " << e.what() << endl;
                        }
                    }
                }
            }
        }
}

void Simulator::storingLabels() {
    int lineNumber = 0;
        for (const string& line : program) {
            // Skip empty lines or comments
            if (line.empty() || line.find_first_not_of(" \t") == string::npos ||
                line[line.find_first_not_of(" \t")] == '#') {
                lineNumber++;
                continue;
            }
            
            istringstream iss(line);
            string first;
            iss >> first;
            
            if (first.back() == ':') {
                string labelName = first.substr(0, first.size()-1);
                labels[labelName] = lineNumber;
                labelPCs[lineNumber] = true;  // Mark this PC as a label
                cout << "Found label: " << labelName << " at line " << lineNumber << endl;
            }
            
            lineNumber++;
        }
}

void Simulator::run() {
    storingLabels();
        parseDataSection(program);
    
        // Pipeline registers for each core.
        vector<PipelineInstruction> if_id(cores.size());
        vector<PipelineInstruction> id_ex(cores.size());
        vector<PipelineInstruction> ex_mem(cores.size());
        vector<PipelineInstruction> mem_wb(cores.size());
    
        vector<int> fetch_pc(cores.size(), 0);
        
        bool program_complete = false;
    
        // Initialize pipeline registers.
        for (int i = 0; i < cores.size(); ++i) {
            if_id[i].valid = false;
            id_ex[i].valid = false;
            ex_mem[i].valid = false;
            mem_wb[i].valid = false;
        }
    
        while (!program_complete) {
            cout << "\nClock cycle: " << clock << "\n";
            cout << "-------------------------------------\n";
    
            // Handle all cores in parallel
            for (int i = 0; i < cores.size(); ++i) {
                // Writeback Stage
                if (mem_wb[i].valid && !mem_wb[i].stalled) {
                    mem_wb[i] = cores[i].writeback(mem_wb[i]);
                    mem_wb[i].valid = false;
                    instructionsExecuted++;
                }
    
                // Memory Stage
if (ex_mem[i].valid && !ex_mem[i].stalled) {
    mem_wb[i] = ex_mem[i];
    cores[i].memory(mem_wb[i], memory);
    ex_mem[i].valid = false;
    
    // Handle control hazard after branch outcome is known
    if (mem_wb[i].isControlInstruction && mem_wb[i].branchTaken) {
        // Branch was taken, need to flush pipeline
        cout << "Core " << i << " detected control hazard - flushing pipeline" << endl;
        controlHazardCount++;
        
        // Flush the invalid instructions
        if (id_ex[i].valid) {
            cout << "Flushing ID/EX: " << id_ex[i].instruction << endl;
            id_ex[i].valid = false;
        }
        
        if (if_id[i].valid) {
            cout << "Flushing IF/ID: " << if_id[i].instruction << endl;
            if_id[i].valid = false;
        }
        
        // Update PC to branch target - this is crucial
        // We need to make sure only this core branches
        int targetPC = cores[i].newPC;
        
        // In a shared fetch implementation, we need special handling
        // Only redirect the fetch PC if this is core 0 (the fetch unit)
        if (i == 0) {
            fetch_pc[0] = targetPC;
            cout << "Core 0 (fetch unit) redirecting to PC = " << fetch_pc[0] << endl;
            
            // Synchronize all other cores' PCs
            for (int j = 1; j < cores.size(); ++j) {
                fetch_pc[j] = targetPC;
            }
        } else {
            // If it's not core 0, we need a different approach
            // We'll keep track of the separate branching core
            cores[i].skipExecution = false;  // This core took the branch, so it should execute at target
            
            // For non-fetch cores that branch, we have a challenge since fetch is shared
            // We need to wait until the shared fetch reaches the target
            cout << "Core " << i << " waiting for fetch to reach target PC " << targetPC << endl;
        }
    }
}
    
                // Execute Stage with Latency
                if (id_ex[i].valid && !id_ex[i].stalled) {
                    // Check if this core should skip execution due to branch
                    if (cores[i].skipExecution) {
                        cout << "Core " << i << " skipping execution of: " << id_ex[i].instruction << endl;
                        
                        // We need to pass this instruction through the pipeline without executing it
                        // Move to next stage but mark as a no-op
                        id_ex[i].valid = false;
                        continue;
                    }
                    // Handle forwarding for instruction in Execute stage
                    if (forwardingEnabled) {
                        // Forward from EX/MEM stage (non-memory instructions)
                        if (ex_mem[i].valid && ex_mem[i].rd != -1 && !ex_mem[i].isMemoryInstruction) {
                            // Forward for rs1
                            if (id_ex[i].rs1 != -1 && id_ex[i].rs1 == ex_mem[i].rd) {
                                id_ex[i].op1 = ex_mem[i].aluResult;
                                cout << "Forwarded value " << ex_mem[i].aluResult 
                                     << " for X" << id_ex[i].rs1 
                                     << " from EX/MEM stage\n";
                            }
                            // Forward for rs2
                            if (id_ex[i].rs2 != -1 && id_ex[i].rs2 == ex_mem[i].rd) {
                                id_ex[i].op2 = ex_mem[i].aluResult;
                                cout << "Forwarded value " << ex_mem[i].aluResult 
                                     << " for X" << id_ex[i].rs2 
                                     << " from EX/MEM stage\n";
                            }
                        }
                        
                        // Forward from MEM/WB stage
                        if (mem_wb[i].valid && mem_wb[i].rd != -1) {
                            // Forward for rs1
                            if (id_ex[i].rs1 != -1 && id_ex[i].rs1 == mem_wb[i].rd) {
                                id_ex[i].op1 = mem_wb[i].aluResult;
                                cout << "Forwarded value " << mem_wb[i].aluResult 
                                     << " for X" << id_ex[i].rs1 
                                     << " from MEM/WB stage\n";
                            }
                            // Forward for rs2
                            if (id_ex[i].rs2 != -1 && id_ex[i].rs2 == mem_wb[i].rd) {
                                id_ex[i].op2 = mem_wb[i].aluResult;
                                cout << "Forwarded value " << mem_wb[i].aluResult 
                                     << " for X" << id_ex[i].rs2 
                                     << " from MEM/WB stage\n";
                            }
                        }
                    }
                    
                    // Handle execution latency
                    if (id_ex[i].execCyclesRemaining > 1) {
                        // Only count a stall if there's actually another instr waiting
                        if (if_id[i].valid) {
                            stallCount++;
                            cout << "Core " << i
                                 << " stalling on EX latency, remaining cycles: "
                                 << id_ex[i].execCyclesRemaining << "\n";
                        }
                        id_ex[i].execCyclesRemaining--;
                    } else {
                        ex_mem[i] = id_ex[i];
                        int branchTaken = cores[i].execute(ex_mem[i], memory, labels, data);
                        id_ex[i].valid = false;
                    }
                }
    
                // Decode and Hazard Detection
                if (if_id[i].valid && !id_ex[i].valid) {
                    PipelineInstruction decoded = cores[i].decode(if_id[i]);
                
                    // Check for hazards
                    bool need_stall = false;
                    
                    // 1) Always check for load-use RAW hazard even with forwarding
                    if (ex_mem[i].valid && ex_mem[i].isLoadInstruction && ex_mem[i].rd != -1) {
                        if ((decoded.rs1 == ex_mem[i].rd && decoded.rs1 != -1) ||
                            (decoded.rs2 == ex_mem[i].rd && decoded.rs2 != -1)) {
                            need_stall = true;
                            cout << "Detected load-use hazard with register X" << ex_mem[i].rd << endl;
                        }
                    }
                    
                    // 2) If forwarding is disabled, also check EX/MEM and MEM/WB RAW hazards
                    if (!forwardingEnabled && !need_stall) {
                        if (ex_mem[i].valid && ex_mem[i].rd != -1 &&
                           ((decoded.rs1 == ex_mem[i].rd && decoded.rs1 != -1) ||
                            (decoded.rs2 == ex_mem[i].rd && decoded.rs2 != -1)))
                        {
                            need_stall = true;
                            cout << "Detected data hazard without forwarding for register X" << ex_mem[i].rd << endl;
                        }
                        if (mem_wb[i].valid && mem_wb[i].rd != -1 &&
                           ((decoded.rs1 == mem_wb[i].rd && decoded.rs1 != -1) ||
                            (decoded.rs2 == mem_wb[i].rd && decoded.rs2 != -1)))
                        {
                            need_stall = true;
                            cout << "Detected data hazard without forwarding for register X" << mem_wb[i].rd << endl;
                        }
                    }
                
                    if (need_stall) {
                        // Count the stall
                        stallCount++;
                        cout << "Stalling core " << i << " due to hazard\n";
                        // Insert a bubble into ID/EX (so EX stage does nothing)
                        id_ex[i].valid = false;
                        // Leave if_id[i] alone so we re-decode the same instruction next cycle
                    } else {
                        // No hazard → advance normally
                        // Set execution latency based on instruction type
                        string op = decoded.opcode;
                        transform(op.begin(), op.end(), op.begin(), ::toupper);
                        decoded.execCyclesRemaining =
                            latencies.count(op) ? latencies[op] : 1;
                
                        id_ex[i] = decoded;
                        if_id[i].valid = false;
                    }
                }
            }                
    
            // Fetch Stage for each core
            if (fetch_pc[0] < program.size()) {
                // Core 0 fetches the instruction
                PipelineInstruction fetched = cores[0].fetch(fetch_pc[0], program);
                
                // If fetched a valid instruction, share it with all cores
                if (fetched.valid) {
                    // Check if this PC corresponds to a label (branch target)
                    bool isLabelPC = labelPCs.count(fetch_pc[0]) > 0;
                    
                    for (int i = 0; i < cores.size(); ++i) {
                        // Clear skipExecution if this is a label PC - this helps cores that should resume execution
                        if (isLabelPC) {
                            cores[i].skipExecution = false;
                            cout << "Core " << i << " reached label at PC=" << fetch_pc[0] << ", resuming execution" << endl;
                        }
                        
                        // Only update if the core's IF/ID stage is ready
                        if (!if_id[i].valid) {
                            // Create a copy of the fetched instruction for this core
                            PipelineInstruction coreInstr = fetched;
                            coreInstr.coreId = i;  // Set the core ID
                            if_id[i] = coreInstr;
                        }
                    }
                    
                    // Advance PC for all cores simultaneously
                    for (int i = 0; i < cores.size(); ++i) {
                        fetch_pc[i] = fetch_pc[0] + 1;
                    }
                } 
                // If not valid (e.g., empty line or comment), just advance PC
                else {
                    for (int i = 0; i < cores.size(); ++i) {
                        fetch_pc[i] = fetch_pc[0] + 1;
                    }
                }
            }
            
    
            clock++;
    
            // Completion check: all pipeline stages must be empty and all PCs at end
            program_complete = true;
            
            for (int i = 0; i < cores.size(); ++i) {
                if (fetch_pc[i] < program.size() || 
                    if_id[i].valid || id_ex[i].valid || ex_mem[i].valid || mem_wb[i].valid) {
                    program_complete = false;
                    break;
                }
            }
        }
    
        cout << "\nSimulation finished after " << clock << " cycles.\n";
    }

void Simulator::display() {
    for (int i = 0; i < cores.size(); ++i) {
        cout << "Core " << i << " Registers: ";
        for (int j = 0; j < 32; ++j)
            cout << setw(3) << cores[i].registers[j] << " ";
        cout << "\n";
    }
    
    cout << "Memory: ";
    for (int i = 0; i < 16; ++i) {
        cout << setw(3) << memory[i] << " ";
    }
    cout << endl;
    
    cout << "Total clock cycles: " << clock << "\n";
    cout << "Total instructions executed: " << instructionsExecuted << "\n";
    cout << "Total data hazard stalls: " << stallCount << "\n";
    cout << "Total control hazard flushes: " << controlHazardCount << "\n";
    
    double ipc = (clock > 0) ? static_cast<double>(instructionsExecuted)/clock : 0;
    cout << "IPC: " << ipc << "\n";
}
