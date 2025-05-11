#include "Simulator.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

int main() {
    vector<string> program;
    string filename;

    cout << "Enter the filename of the assembly program (e.g., program.s): ";
    cin >> filename;

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << "\n";
        return 1;
    }

    string line;
    while (getline(file, line))
        program.push_back(line);

    Simulator sim;
    sim.program = program;
    sim.initializeMemory();

    // Initialize registers for each core.
    for (int i = 0; i < sim.cores.size(); ++i) {
        sim.cores[i].registers[1] = 0;
        sim.cores[i].registers[1] = 1;
        sim.cores[i].registers[2] = 2;
        sim.cores[i].registers[3] = 3;
        sim.cores[i].registers[4] = 4;
        sim.cores[i].registers[5] = 5;
        sim.cores[i].registers[6] = 6;
        sim.cores[i].registers[7] = 7;
        sim.cores[i].registers[8] = 8;
        sim.cores[i].registers[9] = 9;
        sim.cores[i].registers[10] = 10;
    }

    int choice;
    cout << "Enable forwarding? (Enter 1 for Yes, 0 for No): ";
    cin >> choice;
    sim.forwardingEnabled = (choice == 1);

    cout << "Enter latency (in cycles) for ADD: ";
    int lat;
    cin >> lat;
    sim.latencies["ADD"] = lat;

    cout << "Enter latency for SUB: ";
    cin >> lat;
    sim.latencies["SUB"] = lat;

    cout << "Enter latency for ADDI: ";
    cin >> lat;
    sim.latencies["ADDI"] = lat;

    cout << "Enter latency for MUL: ";
    cin >> lat;
    sim.latencies["MUL"] = lat;
    
    // Initialize cache with configuration
    string cacheConfigFile;
    cout << "Enter cache configuration file (leave empty for default): ";
    cin.ignore(); // Clear the newline from previous input
    getline(cin, cacheConfigFile);
    
    sim.initializeCache(cacheConfigFile.empty() ? "cache_config.txt" : cacheConfigFile);

    sim.run();
    cout << "\nProgram size: " << program.size() << "\n";
    sim.display();
    return 0;
}