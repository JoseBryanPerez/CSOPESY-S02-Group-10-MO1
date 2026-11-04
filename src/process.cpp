#include "process.h"

#include <algorithm>
#include <utility>

Process::Process(uint32_t id, std::string name, uint64_t plannedInstructions)
    : id_(id), name_(std::move(name)), total_(plannedInstructions) {}

void Process::setProgram(std::vector<Instruction> program) {
    program_ = std::move(program);
    total_ = program_.size();   // the flattened program is the source of truth for "lines of code"
    loopEnds_.assign(program_.size(), program_.size());
    loopStarts_.assign(program_.size(), program_.size());
    loopStack_.clear();
    std::vector<size_t> starts;
    for (size_t i = 0; i < program_.size(); ++i) {
        if (program_[i].op == OpType::FOR_BEGIN) starts.push_back(i);
        else if (program_[i].op == OpType::FOR_END && !starts.empty()) {
            const size_t start = starts.back();
            starts.pop_back();
            loopEnds_[start] = i;
            loopStarts_[i] = start;
        }
    }
    pc_ = executed_ = 0;
    programReady_ = true;
}

void Process::releaseProgram() {
    std::vector<Instruction>().swap(program_);
    // keep programReady_/total_/pc_ so process-smi / reports still read correctly
}

StepResult Process::executeNext(int coreId) {
    if (pc_ >= program_.size()) return StepResult::Finished;
    const size_t current = static_cast<size_t>(pc_);
    const Instruction& ins = program_[pc_++];
    ++executed_;
    StepResult result = StepResult::Continue;

    if (ins.op == OpType::FOR_BEGIN) {
        const size_t end = loopEnds_[current];
        const uint32_t repeats = ins.a;
        if (end == program_.size()) {
            // Malformed loop marker: safely treat it as a no-op.
        } else if (repeats == 0) {
            pc_ = end + 1;
        } else {
            loopStack_.emplace_back(end, repeats);
        }
    } else if (ins.op == OpType::FOR_END) {
        if (!loopStack_.empty() && loopStack_.back().first == current) {
            auto& frame = loopStack_.back();
            if (--frame.second > 0) {
                pc_ = loopStarts_[current] + 1;
            } else {
                loopStack_.pop_back();
            }
        }
    }

    auto operand = [&](uint32_t value, bool isVar) -> uint32_t {
        return isVar ? var(value) : value;
    };

    switch (ins.op) {
        case OpType::PRINT: {
            LogEntry e{};
            e.when = std::time(nullptr);
            e.core = static_cast<int16_t>(coreId);
            e.textId = ins.a;
            e.hasValue = (ins.flags & kPrintHasVar) != 0;
            e.value = e.hasValue ? var(ins.b) : 0;
            logs_.push_back(e);
            break;
        }
        case OpType::DECLARE:
            var(ins.target) = static_cast<uint16_t>(std::min<uint32_t>(ins.a, UINT16_MAX));
            break;
        case OpType::ADD: {
            uint64_t sum = static_cast<uint64_t>(operand(ins.a, ins.flags & kOperandAIsVar)) +
                           operand(ins.b, ins.flags & kOperandBIsVar);
            var(ins.target) = static_cast<uint16_t>(std::min<uint64_t>(sum, UINT16_MAX));  // clamp high
            break;
        }
        case OpType::SUBTRACT: {
            int64_t diff = static_cast<int64_t>(operand(ins.a, ins.flags & kOperandAIsVar)) -
                           static_cast<int64_t>(operand(ins.b, ins.flags & kOperandBIsVar));
            var(ins.target) = static_cast<uint16_t>(std::max<int64_t>(diff, 0));           // clamp low
            break;
        }
        case OpType::SLEEP:
            sleepTicks_ = static_cast<uint8_t>(ins.a);
            result = StepResult::Sleep;
            break;
        case OpType::FOR_BEGIN:
        case OpType::FOR_END:
            break;
    }

    // A SLEEP that is the final instruction still sleeps first; the scheduler marks the
    // process finished when it is next dispatched (see Process::isComplete).
    if (result == StepResult::Continue && pc_ >= total_) return StepResult::Finished;
    return result;
}
