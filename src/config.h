#pragma once
// Scheduler / process parameters read from config.txt by the "initialize" command.
#include <cstdint>
#include <string>

struct Config {
    // Defaults match the sample config.txt in the spec.
    uint64_t numCpu = 4;               // [1, 128]
    std::string scheduler = "rr";      // "fcfs" or "rr"
    uint64_t quantumCycles = 5;        // [1, 2^32]  (rr only)
    uint64_t batchProcessFreq = 1;     // [1, 2^32]  CPU ticks between generated processes
    uint64_t minIns = 1000;            // [1, 2^32]
    uint64_t maxIns = 2000;            // [1, 2^32]
    uint64_t delaysPerExec = 0;        // [0, 2^32]  busy-wait ticks after each instruction
};

// Reads a space-separated "key value" file. Missing keys keep their defaults.
// Returns false (and fills `error`) if the file can't be opened or a value isn't a number.
// Note: the spec's table says "delays-per-exec" but its sample file says "delay-per-exec";
// both spellings are accepted.
bool loadConfig(const std::string& path, Config& out, std::string& error);

// One-line human readable summary, printed after "initialize".
std::string describeConfig(const Config& cfg);
