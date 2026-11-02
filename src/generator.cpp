#include "generator.h"

#include <algorithm>
#include <string>

namespace {
// Relative weight of SLEEP in the instruction mix (out of 87 total, ~2.3%).
// SLEEP is deliberately rare: every SLEEP sends the process to the BACK of the ready queue,
// so a high rate makes processes crawl (or never finish) when the queue is long.
constexpr int kSleepWeight = 2;
}  // namespace

ProgramGenerator::ProgramGenerator(const Config& cfg) : cfg_(cfg), rng_(std::random_device{}()) {
    for (int i = 0; i < 10; ++i) varPool_.push_back(Symbols::instance().internVar("v" + std::to_string(i)));
}

uint64_t ProgramGenerator::pickLength() {
    uint64_t lo = cfg_.minIns;
    uint64_t hi = std::max(cfg_.minIns, cfg_.maxIns);   // defensive: config isn't range-checked
    if (lo == 0) lo = 1;
    if (hi < lo) hi = lo;
    return std::uniform_int_distribution<uint64_t>(lo, hi)(rng_);
}

Instruction ProgramGenerator::randomInstruction() {
    // Weights: PRINT, DECLARE, ADD, SUBTRACT, SLEEP
    static const int weights[] = {35, 15, 20, 15, kSleepWeight};
    std::discrete_distribution<int> pick(std::begin(weights), std::end(weights));
    auto anyVar = [&]() { return varPool_[rng_() % varPool_.size()]; };
    auto literal = [&]() { return static_cast<uint32_t>(rng_() % 65536); };

    Instruction ins;
    const int choice = pick(rng_);
    switch (choice) {
        case 0:  // PRINT("Hello world from <process_name>!") -- default message, text id 0
            ins.op = OpType::PRINT;
            ins.a = 0;
            break;
        case 1:  // DECLARE(var, value)
            ins.op = OpType::DECLARE;
            ins.target = anyVar();
            ins.a = literal();
            break;
        case 2:  // ADD(var1, var2/value, var3/value)
        case 3:  // SUBTRACT(var1, var2/value, var3/value)
            ins.op = (choice == 2) ? OpType::ADD : OpType::SUBTRACT;
            ins.target = anyVar();
            if (rng_() % 2) { ins.flags |= kOperandAIsVar; ins.a = anyVar(); } else { ins.a = literal(); }
            if (rng_() % 2) { ins.flags |= kOperandBIsVar; ins.b = anyVar(); } else { ins.b = literal(); }
            break;
        default:  // SLEEP(x)
            ins.op = OpType::SLEEP;
            ins.a = 1 + rng_() % 10;   // SLEEP takes a uint8 number of ticks
            break;
    }
    return ins;
}

std::vector<Instruction> ProgramGenerator::generate(uint64_t count) {
    std::vector<Instruction> program;
    program.reserve(count);
    for (uint64_t i = 0; i < count; ++i) program.push_back(randomInstruction());
    return program;
}
