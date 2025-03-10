#include "Simulator.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

int main() {
    vector<string> program;
    ifstream file("program.s");
    if (!file.is_open()){
        cerr << "Error opening file\n";
        return 1;
    }
    string line;
    while (getline(file, line))
        program.push_back(line);

    Simulator sim;
    sim.program = program;
    sim.initializeMemory();

    // Initialize registers for each core.
    for (int i = 0; i < 4; ++i) {
        sim.cores[i].registers[1] = 1;
        sim.cores[i].registers[2] = 2;
        sim.cores[i].registers[3] = 3;
        sim.cores[i].registers[4] = 4;
        sim.cores[i].registers[5] = 5;
        sim.cores[i].registers[6] = 6;
    }

    int choice;
    cout << "Enable forwarding? (Enter 1 for Yes, 0 for No): ";
    cin >> choice;
    sim.forwardingEnabled = (choice == 1);

    // If forwarding is disabled, ask the user for latencies for arithmetic instructions.
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

    sim.run();
    cout << "\nProgram size: " << program.size() << "\n";
    sim.display();
    return 0;
}
