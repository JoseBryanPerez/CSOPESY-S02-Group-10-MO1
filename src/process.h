#pragma once
// A process = a PCB (id, name, state) + a flat instruction list + its variables and print log.
#include <cstdint>
#include <ctime>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include "instruction.h"

enum class ProcessState { Ready, Running, Sleeping, Finished };

// Result of executing one instruction; tells the scheduler what to do with the core.
enum class StepResult {
    Continue,  // keep running on the core
    Sleep,     // SLEEP executed: relinquish the core for sleepTicks() ticks
    Finished,  // that was the last instruction
};

struct LogEntry {
    std::time_t when;
    int16_t core;
    uint32_t textId;   // 0 = default "Hello world from <process>!"
    bool hasValue;     // PRINT("..." + x): value of x at print time
    uint16_t value;
};

class Process {
public:
    Process(uint32_t id, std::string name, uint64_t plannedInstructions);

    // --- identity / state (guarded by the Scheduler's mutex; don't touch from other threads) ---
    uint32_t id() const { return id_; }
    const std::string& name() const { return name_; }
    ProcessState state = ProcessState::Ready;
    int coreId = -1;                 // core currently running this process, -1 if none
    uint64_t wakeTick = 0;           // when Sleeping: CPU tick at which it becomes Ready again
    uint64_t delayRemaining = 0;     // busy-wait ticks left before the next instruction (delays-per-exec)
    std::time_t startTime = 0;       // first time it was dispatched onto a core
    std::time_t finishTime = 0;

    // --- program ---
    // Programs are generated lazily on first dispatch so that thousands of queued processes
    // don't each hold a 1000+ instruction vector. Until then, totalInstructions() is the plan.
    bool hasProgram() const { return programReady_; }
    void setProgram(std::vector<Instruction> program);
    void releaseProgram();            // called when finished, frees the instruction memory

    uint64_t instructionsExecuted() const { return executed_; }   // includes repeated loop body steps
    uint64_t totalInstructions() const { return total_; }   // "Lines of code"
    uint64_t getCurrentLine() const { return pc_ + 1; }
    bool isComplete() const { return programReady_ && pc_ >= total_; }

    // Executes exactly ONE instruction on `coreId`. Precondition: hasProgram() && !isComplete().
    StepResult executeNext(int coreId);
    uint8_t sleepTicks() const { return sleepTicks_; }       // valid right after StepResult::Sleep

    const std::vector<LogEntry>& logs() const { return logs_; }

private:
    uint16_t& var(uint32_t id) { return vars_[static_cast<uint16_t>(id)]; }   // auto-declares as 0

    uint32_t id_;
    std::string name_;
    std::vector<Instruction> program_;
    bool programReady_ = false;
    uint64_t pc_ = 0;
    uint64_t executed_ = 0;
    std::vector<size_t> loopEnds_;
    std::vector<size_t> loopStarts_;
    std::vector<std::pair<size_t, uint32_t>> loopStack_; // FOR_END index and remaining iterations
    uint64_t total_;
    uint8_t sleepTicks_ = 0;
    std::unordered_map<uint16_t, uint16_t> vars_;   // uint16 variables, clamped to [0, 65535]
    std::vector<LogEntry> logs_;
};
