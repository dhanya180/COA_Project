#include "CacheConfigReader.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

ReplacementPolicy CacheConfigReader::parseReplacementPolicy(const string& policyStr) {
    string policy = policyStr;
    transform(policy.begin(), policy.end(), policy.begin(), ::toupper);
    
    if (policy == "LRU") {
        return ReplacementPolicy::LRU;
    } else if (policy == "FIFO") {
        return ReplacementPolicy::FIFO;
    } else if (policy == "RANDOM") {
        return ReplacementPolicy::RANDOM;
    } else {
        cout << "Unknown replacement policy: " << policyStr << ", defaulting to LRU" << endl;
        return ReplacementPolicy::LRU;
    }
}

CacheConfig CacheConfigReader::readConfig(const string& filename) {
    CacheConfig config;
    
    // Default values
    config.l1i_size = 32768;       // 32 KB
    config.l1i_block_size = 64;    // 64 bytes
    config.l1i_assoc = 4;          // 4-way
    config.l1i_latency = 1;        // 1 cycle
    
    config.l1d_size = 32768;       // 32 KB
    config.l1d_block_size = 64;    // 64 bytes
    config.l1d_assoc = 8;          // 8-way
    config.l1d_latency = 1;        // 1 cycle
    
    config.l2_size = 262144;       // 256 KB
    config.l2_block_size = 64;     // 64 bytes
    config.l2_assoc = 8;           // 8-way
    config.l2_latency = 10;        // 10 cycles
    
    config.main_memory_latency = 100;  // 100 cycles
    config.policy = ReplacementPolicy::LRU;
    
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Warning: Could not open cache config file: " << filename << endl;
        cout << "Using default cache configuration." << endl;
        return config;
    }
    
    string line;
    while (getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t delimPos = line.find('=');
        if (delimPos == string::npos) {
            continue;
        }
        
        string key = line.substr(0, delimPos);
        string value = line.substr(delimPos + 1);
        
        // Remove whitespace
        key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(remove_if(value.begin(), value.end(), ::isspace), value.end());
        
        // Parse configuration
        if (key == "L1I_SIZE") {
            config.l1i_size = stoi(value);
        } else if (key == "L1I_BLOCK_SIZE") {
            config.l1i_block_size = stoi(value);
        } else if (key == "L1I_ASSOC") {
            config.l1i_assoc = stoi(value);
        } else if (key == "L1I_LATENCY") {
            config.l1i_latency = stoi(value);
        } else if (key == "L1D_SIZE") {
            config.l1d_size = stoi(value);
        } else if (key == "L1D_BLOCK_SIZE") {
            config.l1d_block_size = stoi(value);
        } else if (key == "L1D_ASSOC") {
            config.l1d_assoc = stoi(value);
        } else if (key == "L1D_LATENCY") {
            config.l1d_latency = stoi(value);
        } else if (key == "L2_SIZE") {
            config.l2_size = stoi(value);
        } else if (key == "L2_BLOCK_SIZE") {
            config.l2_block_size = stoi(value);
        } else if (key == "L2_ASSOC") {
            config.l2_assoc = stoi(value);
        } else if (key == "L2_LATENCY") {
            config.l2_latency = stoi(value);
        } else if (key == "MAIN_MEMORY_LATENCY") {
            config.main_memory_latency = stoi(value);
        } else if (key == "REPLACEMENT_POLICY") {
            config.policy = parseReplacementPolicy(value);
        } else {
            cout << "Unknown cache configuration parameter: " << key << endl;
        }
    }
    
    cout << "Loaded cache configuration from " << filename << endl;
    return config;
}