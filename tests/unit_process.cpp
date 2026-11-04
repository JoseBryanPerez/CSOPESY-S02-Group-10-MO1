// Tiny dependency-free unit test for instruction semantics. Build+run: make unit
#include <cstdio>
#include "../src/process.h"

static int failures = 0;
#define EXPECT(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } else std::printf("PASS: %s\n", msg); } while (0)

static Instruction ins(OpType op, uint16_t target, uint32_t a, uint32_t b, uint8_t flags = 0) {
    Instruction i; i.op = op; i.target = target; i.a = a; i.b = b; i.flags = flags; return i;
}

int main() {
    auto& S = Symbols::instance();
    uint16_t x = S.internVar("x"), y = S.internVar("y");

    Process p(1, "t", 8);
    p.setProgram({
        ins(OpType::DECLARE, x, 65000, 0),                                   // x = 65000
        ins(OpType::ADD, x, x, 1000, kOperandAIsVar),                        // x = clamp(66000) = 65535
        ins(OpType::SUBTRACT, y, 5, 10),                                     // y = clamp(5-10) = 0
        ins(OpType::SUBTRACT, y, x, 35, kOperandAIsVar),                     // y = 65500
        ins(OpType::PRINT, 0, S.internText("Value from: "), y, kPrintHasVar),
        ins(OpType::PRINT, 0, 0, 0),                                         // default message
        ins(OpType::SLEEP, 0, 7, 0),
        ins(OpType::ADD, 3, 1, 2),                                           // var 3 auto-declared, = 3
    });

    for (int i = 0; i < 6; ++i) EXPECT(p.executeNext(2) == StepResult::Continue, "step continues");
    EXPECT(p.logs().size() == 2, "two PRINTs logged");
    EXPECT(p.logs()[0].hasValue && p.logs()[0].value == 65500, "PRINT appends variable value (and clamping worked)");
    EXPECT(p.logs()[0].core == 2, "log records the core");
    EXPECT(p.logs()[1].textId == 0 && !p.logs()[1].hasValue, "default PRINT uses default message");
    EXPECT(p.executeNext(2) == StepResult::Sleep && p.sleepTicks() == 7, "SLEEP(7) relinquishes the core");
    EXPECT(p.executeNext(0) == StepResult::Finished, "last instruction finishes the process");
    EXPECT(p.instructionsExecuted() == 8 && p.isComplete(), "pc == lines of code at the end");

    const uint16_t loopVar = S.internVar("loopVar");
    Process loop(2, "loop", 0);
    loop.setProgram({
        ins(OpType::DECLARE, loopVar, 0, 0),
        ins(OpType::FOR_BEGIN, 0, 3, 0),
        ins(OpType::ADD, loopVar, loopVar, 1, kOperandAIsVar),
        ins(OpType::FOR_END, 0, 0, 0),
        ins(OpType::PRINT, 0, S.internText("loop result: "), loopVar, kPrintHasVar),
    });
    StepResult loopResult = StepResult::Continue;
    while (loopResult == StepResult::Continue) loopResult = loop.executeNext(1);
    EXPECT(loopResult == StepResult::Finished, "FOR program finishes");
    EXPECT(loop.logs().size() == 1 && loop.logs()[0].value == 3,
           "FOR repeats its body the requested number of times");
    EXPECT(loop.instructionsExecuted() == 9,
           "executed instruction count includes each repeated body pass");

    Process nested(3, "nested", 0);
    nested.setProgram({
        ins(OpType::DECLARE, loopVar, 0, 0),
        ins(OpType::FOR_BEGIN, 0, 2, 0),
        ins(OpType::FOR_BEGIN, 0, 3, 0),
        ins(OpType::ADD, loopVar, loopVar, 1, kOperandAIsVar),
        ins(OpType::FOR_END, 0, 0, 0),
        ins(OpType::FOR_END, 0, 0, 0),
        ins(OpType::PRINT, 0, S.internText("nested result: "), loopVar, kPrintHasVar),
    });
    StepResult nestedResult = StepResult::Continue;
    while (nestedResult == StepResult::Continue) nestedResult = nested.executeNext(0);
    EXPECT(nestedResult == StepResult::Finished && nested.logs().size() == 1 &&
           nested.logs()[0].value == 6, "nested FOR loops repeat inner body for each outer pass");
    return failures ? 1 : 0;
}
