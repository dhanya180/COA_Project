#include "Simulator.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace std;

Simulator::Simulator() : memory(1024, 0), clock(0), cores(4, Cores(0)),
    stallCount(0), controlHazardCount(0), instructionsExecuted(0), 
    forwardingEnabled(true), memoryStalls(0) {
    for (int i = 0; i < 4; ++i)
        cores[i] = Cores(i);
}




void Simulator::initializeCache(const string& configFile) {
    CacheConfigReader reader;
    CacheConfig config = reader.readConfig(configFile);
    
    // Initialize cache hierarchy with the config
    cacheHierarchy = make_unique<CacheHierarchy>(
        config.l1i_size, config.l1i_block_size, config.l1i_assoc, config.l1i_latency,
        config.l1d_size, config.l1d_block_size, config.l1d_assoc, config.l1d_latency,
        config.l2_size, config.l2_block_size, config.l2_assoc, config.l2_latency,
        config.main_memory_latency, config.policy
    );
    for (auto& core : cores) {
    core.cacheHierarchy = cacheHierarchy.get();
}
    
    cout << "Cache hierarchy initialized successfully." << endl;
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
                        cout << "Stored " << value << " at Mem[" << memIndex << "]\n";
                        int baseIndex = memIndex ;
                        cout<<"Base Index: "<<baseIndex<<endl;
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
    bool inTextSection = false;
    int textStartIndex = -1;

    // Locate where .text starts
    for (int i = 0; i < program.size(); ++i) {
        if (program[i].find(".text") != string::npos) {
            textStartIndex = i + 1;
            break;
        }
    }

    cout << "------------------ PC to Instruction Mapping ------------------" << endl;

    for (int i = textStartIndex; i < program.size(); ++i) {
        string line = program[i];

        // Skip comments and empty lines
        if (line.empty() || line.find_first_not_of(" \t") == string::npos || line[line.find_first_not_of(" \t")] == '#') {
            continue;
        }

        istringstream iss(line);
        string first;
        iss >> first;

        // If it's a label, mark its location
        if (first.back() == ':') {
            string labelName = first.substr(0, first.size() - 1);
            labels[labelName] = lineNumber;
            labelPCs[lineNumber] = true;
            cout << "[PC = " << lineNumber << "]  " << labelName << ":" << endl;
        } else {
            cout << "[PC = " << lineNumber << "]  " << line << endl;
            lineNumber++;
        }
    }
    cout << "-------------------------------------------------------------" << endl;
}



void Simulator::run() {
    storingLabels();
    parseDataSection(program);
    
    // Pipeline registers for each core.
    vector<PipelineInstruction> if_id(cores.size());
    vector<PipelineInstruction> id_ex(cores.size());
    vector<PipelineInstruction> ex_mem(cores.size());
    vector<PipelineInstruction> mem_wb(cores.size());

    int textStartIndex = -1;
    for (int i = 0; i < program.size(); ++i) {
        if (program[i].find(".text") != string::npos) {
            textStartIndex = i + 1;
            break;
        }
    }

    vector<string> textSegment;

    if (textStartIndex != -1) {
        // .text found → trim to just code
        for (int i = textStartIndex; i < program.size(); ++i)
            textSegment.push_back(program[i]);
    } else {
        // .text not found → assume everything is code except .data section
        bool inData = false;
        for (const string& line : program) {
            if (line.find(".data") != string::npos) {
                inData = true;
                continue;
            }
            if (line.find(".word") != string::npos || line.find(".string") != string::npos)
                continue;
            if (inData && line.find(":") != string::npos)
                continue;

            textSegment.push_back(line);
        }
    }

    program = textSegment;
    vector<int> fetch_pc(cores.size(), 0);
    
    bool program_complete = false;
    
    // Initialize pipeline registers.
    for (int i = 0; i < cores.size(); ++i) {
        if_id[i].valid = false;
        id_ex[i].valid = false;
        ex_mem[i].valid = false;
        mem_wb[i].valid = false;
    }
    
    // Flag to indicate a branch was taken in the previous cycle
    bool branch_taken_previous_cycle = false;
    
    while (!program_complete) {
        cout << "\nClock cycle: " << clock << "\n";
        cout << "-------------------------------------\n";
        
        // Handle all cores in parallel
        for (int i = 0; i < cores.size(); ++i) {
            // Writeback Stage
            if (mem_wb[i].valid && !mem_wb[i].stalled) {
                // Special handling for SYNC instruction
                if (mem_wb[i].isSyncInstruction) {
                    // SYNC instruction completes, move to next instruction
                    mem_wb[i].valid = false;
                    instructionsExecuted++;
                    continue;
                }
                
                mem_wb[i] = cores[i].writeback(mem_wb[i]);
                mem_wb[i].valid = false;
                instructionsExecuted++;
            }
            

            // Memory Stage
            if (ex_mem[i].valid && !ex_mem[i].stalled) {
                // Special handling for SYNC instruction
                if (ex_mem[i].isSyncInstruction) {
                    // If sync is not complete, stall this stage
                    if (cores[i].sync_phase_complete) {
                        // Move to writeback
                        mem_wb[i] = ex_mem[i];
                        ex_mem[i].valid = false;
                    } else {
                        // Keep stalled
                        stallCount++;
                        continue;
                    }
                } else {
                    // Check if we're still waiting for memory access to complete
                    if (ex_mem[i].waitingForMemory) {
                        if (ex_mem[i].memoryCycles > 0) {
                            ex_mem[i].memoryCycles--;
                            cout << "Core " << i << " waiting for memory access, "
                                << ex_mem[i].memoryCycles << " cycles remaining" << endl;
                            stallCount++;
                            continue;
                        } else {
                            // Memory access is complete, proceed to WB stage
                            ex_mem[i].waitingForMemory = false;
                        }
                    }
                    
                    mem_wb[i] = ex_mem[i];
                    
                    
                    // Access memory through cache if it's a load/store
                    if (mem_wb[i].isMemoryInstruction) {
                        int address = mem_wb[i].memoryAddress;
                        bool isWrite = mem_wb[i].opcode == "SW" || mem_wb[i].opcode == "SB" || 
                                    mem_wb[i].opcode == "SH" || mem_wb[i].opcode == "SD";
                        
                        // Access data cache
                        int cacheCycles = cacheHierarchy->accessDataCache(address, isWrite, memory);
                        
                        // Account for cache latency
                        if (cacheCycles > 1) {
                            ex_mem[i].waitingForMemory = true;
                            ex_mem[i].memoryCycles = cacheCycles - 1; // -1 because we're using one cycle now
                            stallCount++;
                            cout << "Core " << i << " memory access through cache, latency: " 
                                << cacheCycles << " cycles" << endl;
                            continue;  // Skip to next iteration
                        }
                    }
                    if (mem_wb[i].opcode == "LW_SPM" || mem_wb[i].opcode == "SW_SPM") {
                        // For SPM instructions, we handle the access directly through executeSPMAccess
                        // which was called in the execute stage
                        int address = mem_wb[i].memoryAddress;
                        bool isWrite = mem_wb[i].opcode == "SW_SPM";
                        
                        // If we need to wait for SPM access to complete
                        if (mem_wb[i].memoryCycles > 1) {
                            ex_mem[i].waitingForMemory = true;
                            ex_mem[i].memoryCycles = mem_wb[i].memoryCycles - 1;
                            stallCount++;
                            cout << "Core " << i << " waiting for SPM access, "
                                << ex_mem[i].memoryCycles << " cycles remaining" << endl;
                            continue;  // Skip to next iteration
                        }
                    }

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
                        ex_mem[i].valid   = false;
                        mem_wb[i].valid   = false;
                        
                        // Update PC to branch target - this is crucial
                        // We need to make sure only this core branches
                        int targetPC = cores[i].newPC;
                        
                        // Set the flag to indicate a branch was taken this cycle
                        branch_taken_previous_cycle = true;
                        
                        // In a shared fetch implementation, we need special handling
                        // Only redirect the fetch PC if this is core mox0 (the fetch unit)
                        if (i == 0) {
                            // This is the important fix - set PC directly to target without incrementing
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
            }
            // Execute Stage with Latency
            if (id_ex[i].valid && !id_ex[i].stalled) {
                // Special handling for SYNC instruction
                if (id_ex[i].isSyncInstruction) {
                    // SYNC instruction goes directly to memory stage
                    ex_mem[i] = id_ex[i];
                    id_ex[i].valid = false;
                } else {
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
                        
                        // For memory instructions, calculate the memory address here
                        if (ex_mem[i].opcode == "LW" || ex_mem[i].opcode == "SW" || 
                            ex_mem[i].opcode == "LB" || ex_mem[i].opcode == "SB" ||
                            ex_mem[i].opcode == "LH" || ex_mem[i].opcode == "SH" ||
                            ex_mem[i].opcode == "LD" || ex_mem[i].opcode == "SD") {
                            
                            // For load/store instructions, the address is typically rs1 + immediate
                            int baseAddr = id_ex[i].op1;  // Value in rs1
                            int offset = id_ex[i].imm;    // Immediate value
                            ex_mem[i].memoryAddress = baseAddr + offset;
                        }
                        
                        int branchTaken = cores[i].execute(ex_mem[i], memory, labels, data);
                        id_ex[i].valid = false;
                    }
                }
            }
            // Decode and Hazard Detection
            if (if_id[i].valid && !id_ex[i].valid) {
                PipelineInstruction decoded = cores[i].decode(if_id[i]);
                
                // Special handling for SYNC instruction
                if (decoded.isSyncInstruction) {
                    // SYNC instruction can go directly to execute stage
                    id_ex[i] = decoded;
                    if_id[i].valid = false;
                } else {
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
        }
        
        // Fetch Stage with Cache
        if (fetch_pc[0] < program.size()) {
            // Access instruction cache for fetch
            int cacheStallCycles = 0;
            
            // Only do cache access if it's a valid instruction (not empty line/comment)
            if (!program[fetch_pc[0]].empty() && 
                program[fetch_pc[0]].find_first_not_of(" \t") != string::npos &&
                program[fetch_pc[0]][program[fetch_pc[0]].find_first_not_of(" \t")] != '#') {
                
                // Convert PC to memory address (assuming 4 bytes per instruction)
                int memAddress = fetch_pc[0] * 4;
                
                // Access instruction cache
                cacheStallCycles = cacheHierarchy->accessInstructionCache(memAddress, memory);
                
                if (cacheStallCycles > 1) {
                    cout << "I-cache access latency: " << cacheStallCycles << " cycles" << endl;
                    // We'll stall the fetch for cacheStallCycles-1 (one cycle used now)
                    stallCount += (cacheStallCycles - 1);
                    
                    // Just account for the stall but don't actually stall the simulator
                    // in this implementation for simplicity - we'll pretend the cache 
                    // was a hit and continue simulation
                }
            }
            
            // Core 0 fetches the instruction
            PipelineInstruction fetched = cores[0].fetch(fetch_pc[0], program);
            
            // If fetched a valid instruction, share it with all cores
            if (fetched.valid) {
                // Check if this PC corresponds to a label (branch target)
                bool isLabelPC = labelPCs.count(fetch_pc[0]) > 0;
                

                // If we just took a branch, print debug info about the target
                if (branch_taken_previous_cycle) {
                    cout << "Successfully fetched branch target at PC=" << fetch_pc[0] << ": " 
                        << fetched.instruction << endl;
                }
                
                // Flag to check if any core accepted the instruction
                bool anyCoreTookInstruction = false;

                for (int i = 0; i < cores.size(); ++i) {
                    // Clear skipExecution if this is a label PC - this helps cores that should resume execution
                    if (isLabelPC) {
                        cores[i].skipExecution = false;
                        cout << "Core " << i << " reached label at PC=" << fetch_pc[0] << ", resuming execution" << endl;
                    }
                    
                    // Only update if the core's IF/ID stage is ready
                    if (!if_id[i].valid && !cores[i].skipExecution) {
                         PipelineInstruction coreInstr = fetched;
                        coreInstr.coreId = i;  // Set the core ID
                        if_id[i] = coreInstr;
                        anyCoreTookInstruction = true;
                
                cout << "Core " << i << " accepted instruction at PC=" << fetch_pc[0] << endl;
                    }
                }
                
                // KEY FIX HERE: Only increment PC if no branch was taken in the previous cycle
                if (!branch_taken_previous_cycle || anyCoreTookInstruction) {
                    // Advance PC for all cores simultaneously
                    for (int i = 0; i < cores.size(); ++i) {
                        fetch_pc[i] = fetch_pc[0] + 1;
                    }
                     // If we were at a branch target, we've now processed it
            if (branch_taken_previous_cycle) {
                branch_taken_previous_cycle = false;
                cout << "Moving past branch target to PC=" << fetch_pc[0] << endl;
            }

                }  else {
            // We're still at the branch target but no cores took the instruction yet
            cout << "Remaining at branch target PC=" << fetch_pc[0] 
                 << " since no cores were ready to accept it" << endl;
        }
            } 
            // If not valid (e.g., empty line or comment), just advance PC if not after a branch
            else {
                if (!branch_taken_previous_cycle) {
                    for (int i = 0; i < cores.size(); ++i) {
                        fetch_pc[i] = fetch_pc[0] + 1;
                    }
                } else {
                     // We're at the branch target but it's an invalid instruction
                    // This could be a problem - print a warning
                    cout << "WARNING: Branch target at PC=" << fetch_pc[0] 
                        << " appears to be an invalid instruction!" << endl;
                    // Still clear the branch_taken flag to avoid infinite loop
                    // Reset branch taken flag
                    branch_taken_previous_cycle = false;
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
    
    // Collect final cache statistics
    memoryStalls = cacheHierarchy->getTotalMemoryStalls();
    
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
    for (int i = 0; i < 1024; ++i) {
        cout << setw(2) << memory[i] << " ";
    }
    cout << endl;
    
    cout << "Total Pipline clock cycles: " << clock << "\n";
    //cout << "Total instructions executed: " << instructionsExecuted << "\n";
    cout << "Total data hazard stalls: " << stallCount << "\n";
    cout << "Total control hazard flushes: " << controlHazardCount << "\n";
    cout << "Total memory stalls from cache: " << memoryStalls << "\n";
    
    double ipc = (clock > 0) ? static_cast<double>(instructionsExecuted)/clock : 0;
    cout << "IPC: " << ipc << "\n";
    cout << "Scratchpad Memory:" << endl;
    cacheHierarchy->getScratchpadMemory()->printContents();
    // Print cache statistics
    cacheHierarchy->printStats();
}