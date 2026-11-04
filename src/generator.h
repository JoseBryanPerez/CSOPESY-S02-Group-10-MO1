#pragma once
// Builds randomized instruction lists for dummy processes (scheduler-start).
#include <cstdint>
#include <random>
#include <vector>

#include "config.h"
#include "instruction.h"

class ProgramGenerator {
public:
    explicit ProgramGenerator(const Config& cfg);

    // Random program length in [min-ins, max-ins].
    uint64_t pickLength();

    // Builds a program of exactly `count` instructions.
    std::vector<Instruction> generate(uint64_t count);

private:
    Instruction randomInstruction(unsigned loopDepth);
    void generateInto(std::vector<Instruction>& program, uint64_t count, unsigned loopDepth);

    Config cfg_;
    std::mt19937_64 rng_;
    std::vector<uint16_t> varPool_;   // interned ids for v0..v9
};
