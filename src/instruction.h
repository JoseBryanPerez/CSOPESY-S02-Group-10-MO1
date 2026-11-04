#pragma once
// Instruction set for emulated processes.
//
//   PRINT(msg)                    DECLARE(var, value)
//   ADD(var1, var2/val, var3/val) SUBTRACT(var1, var2/val, var3/val)
//   SLEEP(x)
//
//
// Design note: the sample config spawns a new 1000-2000 instruction process *every tick*,
// so memory per instruction matters. An Instruction is therefore a flat 12-byte record
// (variable names and custom PRINT text live in the Symbols table, not in the instruction).
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// FOR is stored as a FOR_BEGIN marker (a = repeat count) and matching FOR_END marker.
// The markers let programs stay compact and keep the instruction record flat.
enum class OpType : uint8_t { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR_BEGIN, FOR_END };

enum InstructionFlags : uint8_t {
    kOperandAIsVar = 1 << 0,  // ADD/SUBTRACT: operand `a` is a variable id (else literal)
    kOperandBIsVar = 1 << 1,  // ADD/SUBTRACT: operand `b` is a variable id (else literal)
    kPrintHasVar   = 1 << 2,  // PRINT: append the value of variable `b` to the message
};

struct Instruction {
    OpType op = OpType::PRINT;
    uint8_t flags = 0;
    uint16_t target = 0;  // DECLARE/ADD/SUBTRACT: destination variable id
    uint32_t a = 0;       // DECLARE: value | ADD/SUB: literal or var id | PRINT: text id (0 = default msg)
                          // SLEEP: number of ticks (uint8)
    uint32_t b = 0;       // ADD/SUB: literal or var id | PRINT: var id when kPrintHasVar
};

// Interns variable names and custom PRINT strings so instructions can stay tiny.
// Thread-safe (internal mutex); ids are stable for the life of the program.
class Symbols {
public:
    static Symbols& instance();

    uint16_t internVar(const std::string& name);
    std::string varName(uint16_t id) const;

    // id 0 is reserved for "default message" (Hello world from <process_name>!)
    uint32_t internText(const std::string& text);
    std::string text(uint32_t id) const;

private:
    Symbols();
    mutable std::mutex mtx_;
    std::unordered_map<std::string, uint16_t> varIds_;
    std::vector<std::string> varNames_;
    std::unordered_map<std::string, uint32_t> textIds_;
    std::vector<std::string> texts_;
};
