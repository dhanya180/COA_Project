#ifndef CACHE_CONFIG_READER_HPP
#define CACHE_CONFIG_READER_HPP

#include "Cache.hpp"
#include <string>
#include <unordered_map>

struct CacheConfig {
    int l1i_size;
    int l1i_block_size;
    int l1i_assoc;
    int l1i_latency;
    
    int l1d_size;
    int l1d_block_size;
    int l1d_assoc;
    int l1d_latency;
    
    int l2_size;
    int l2_block_size;
    int l2_assoc;
    int l2_latency;
    
    int main_memory_latency;
    ReplacementPolicy policy;
};

class CacheConfigReader {
public:
    static CacheConfig readConfig(const std::string& filename);
    static ReplacementPolicy parseReplacementPolicy(const std::string& policyStr);
};

#endif // CACHE_CONFIG_READER_HPP