#pragma once
#include "scheduler.h"

class RoundRobinScheduler : public Scheduler {
public:
    RoundRobinScheduler(const Config& cfg, uint64_t tickIntervalUs);
private:
    void dispatchCore(Core& core) override;
    void runCores() override;
    void requeueProcess(Core& core);

    uint64_t quantumCycles_;
};
