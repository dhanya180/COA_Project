# 🚀 RISC-V Simulator
A multi-core RISC-V processor simulator capable of simulating multiple cores with shared memory, pipelining, cache hierarchies, and specialized memory systems.

## 📋 Project Overview
This simulator was developed as part of the CS209P Computer Architecture course project, completed in three phases:

### Phase 1: Multi-Core Base Implementation ⚙️

* Four independent RISC-V processor cores with shared memory access
* Support for core-specific instruction sets including ADD/SUB, BNE, JAL, LW/SW
* Each core has access to dedicated memory regions within a shared 4kB memory space
* Core identification through special purpose read-only registers

### Phase 2: Pipelining and Shared Execution 🔄

* Implementation of pipelining with data forwarding capability (can be toggled)
* Single fetch unit shared across all compute units, with separate decode/execute/memory/writeback stages
* Variable instruction latencies configurable by users
* Conditional execution based on core ID (CID)
* Performance metrics including stall counts and IPC (Instructions Per Cycle)

### Phase 3: Memory Hierarchy and Synchronization 🧠

* Complete memory hierarchy with L1 instruction cache, L1 data cache, and unified L2 cache
* Configurable cache parameters (size, block size, associativity, access latency)
* Multiple cache replacement policies (LRU and another chosen policy)
* Scratchpad memory (SPM) implementation with specialized instructions
* SYNC instruction for core synchronization
* Comprehensive performance metrics (stalls, cache miss rates, IPC)
A multi-core RISC-V processor simulator capable of simulating multiple cores with shared memory, pipelining, cache hierarchies, and specialized memory systems.

## 🛠️ Setup and Installation

### Installation
```
git clone https://github.com/dhanya180/RISC-V_Simulator.git
cd RISC-V_Simulator
```

## 🎮 Usage

### Running the Simulator
```
g++ main.cpp Simulator.cpp Cores.cpp CacheConfigReader.cpp CacheHierarchy.cpp Cache.cpp ScratchpadMemory.cpp -o Simulator
./Simulator
```

## ✨ Features

### Supported RISC-V Instructions

* ```ADD/SUB/MUL/ADDI```: Basic arithmetic operations
* ```BNE/BLE/BEQ```: Branch if not equal
* ```JAL/J```: Jump and link for function calls
* ```LW/SW```: Load/store words to memory
* ```NOP```: No operation
* ```ECALL```: Environment call for system functions
* ```LI```: Load immediate
* ```LA```: Load address
* ```SYNC```: Synchronization between cores (Phase 3)
* ```LW_SPM/SW_SPM```: Scratchpad memory operations (Phase 3)
* Custom instruction: [Describe any custom instruction implemented]

### Memory System

* 4kB shared memory space with core-specific regions
* Two-level cache hierarchy (Phase 3)
* Scratchpad memory for programmer-controlled data placement (Phase 3)

### Performance Analysis

The simulator provides the following performance metrics:
* Instruction count
* Cycle count
* Stall count
* IPC (Instructions Per Cycle)
* Cache miss rates (Phase 3)
