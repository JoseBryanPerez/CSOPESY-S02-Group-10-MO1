#include "round_robin_scheduler.h"

#include "config.h"
#include "generator.h"
#include "process.h"

RoundRobinScheduler::RoundRobinScheduler(const Config& cfg, uint64_t tickIntervalUs)
    : Scheduler(cfg, tickIntervalUs), quantumCycles_(cfg.quantumCycles) {
    //Already initialized in Scheduler
}

void RoundRobinScheduler::runCores() {
    for (Core& core: cores_) {
        if (!core.busy()) continue;
        Process& p = *core.proc;

        if (p.isComplete()) {
            finishProcess(core);
            core.ticksRan = 0;
            continue;
        }

        if (p.delayRemaining > 0) {
            --p.delayRemaining;
            continue;
        }

        StepResult r = p.executeNext(core.id);
        p.delayRemaining = cfg_.delaysPerExec;
        core.ticksRan++;
        if (r == StepResult::Finished) {
            finishProcess(core);
            core.ticksRan = 0;
        } else if (r == StepResult::Sleep) {
            sleepProcess(core, p.sleepTicks());
            core.ticksRan = 0;
        } else if (core.ticksRan >= quantumCycles_) {
            requeueProcess(core);
            core.ticksRan = 0;
        }
    }
}

void RoundRobinScheduler::requeueProcess(Core& core) {
    std::shared_ptr<Process> p = core.proc;
    p->state = ProcessState::Ready;
    p->coreId = -1;
    p->delayRemaining = 0;
    ready_.push_back(p);
    core.proc.reset();
}