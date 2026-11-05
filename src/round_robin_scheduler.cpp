#include "round_robin_scheduler.h"

#include "config.h"
#include "generator.h"
#include "process.h"

RoundRobinScheduler::RoundRobinScheduler(const Config& cfg, uint64_t tickIntervalUs)
    : Scheduler(cfg, tickIntervalUs), quantumCycles_(cfg.quantumCycles) {
    //Already initialized in Scheduler
}

void RoundRobinScheduler::dispatchCore(Core& core) {
    if (ready_.empty()) return;

    std::shared_ptr<Process> p = ready_.front();
    ready_.pop_front();

    core.proc = p;
    core.ticksRan = 0;
    p->state = ProcessState::Running;
    p->coreId = core.id;
    if (p->startTime == 0) p->startTime = std::time(nullptr);
    if (!p->hasProgram()) p->setProgram(generator_.generate(p->totalInstructions()));
}

void RoundRobinScheduler::runCores() {
    for (Core& core: cores_) {
        if (!core.busy()) {
            dispatchCore(core);
            if (!core.busy()) continue;
        }

        Process& p = *core.proc;

        if (p.isComplete()) {
            finishProcess(core);
            dispatchCore(core);
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
            dispatchCore(core);
        } else if (r == StepResult::Sleep) {
            sleepProcess(core, p.sleepTicks());
            dispatchCore(core);
        } else if (core.ticksRan >= quantumCycles_) {
            requeueProcess(core);
            dispatchCore(core);
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