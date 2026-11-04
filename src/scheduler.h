#pragma once
// The CPU scheduler: a real-time tick loop running on its own thread.
//
// Every tick (one "frame pass" of cpuCycles in the spec's pseudocode) the loop:
//   1. increments cpuCycles
//   2. spawns a dummy process if scheduler-start is active and it's time (batch-process-freq)
//   3. wakes SLEEPing processes whose time is up
//   4. dispatches ready processes onto idle cores
//   5. runs one tick on every busy core (one instruction, or one tick of delays-per-exec busy-wait)
//
// The CLI runs on the main thread and talks to the scheduler through the public methods
// below, all of which take the internal mutex. Only the CLI thread ever prints.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "generator.h"
#include "process.h"

struct Core {
    int id = 0;
    uint64_t ticksRan = 0;
    std::shared_ptr<Process> proc;   // null = idle
    bool busy() const { return proc != nullptr; }
};

class Scheduler {
public:
    Scheduler(const Config& cfg, uint64_t tickIntervalUs);
    ~Scheduler();
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void start();   // launches the tick thread
    void stop();    // joins it

    // scheduler-start / scheduler-stop
    void setGenerating(bool on);
    bool generating() const;

    // Text for "screen -ls" and the contents of csopesy-log.txt (mockup lines 38-54).
    std::string buildUtilizationReport() const;
    std::string buildLogReportFromProcess(const std::string& processName, bool excludeFinished) const;

    uint64_t cpuCycles() const;
    const Config& config() const { return cfg_; }

protected:
    virtual void runCores();
    void finishProcess(Core& core);
    void sleepProcess(Core& core, uint8_t ticks);
    
    Config cfg_;
    uint64_t tickIntervalUs_;
    ProgramGenerator generator_;

    mutable std::mutex mtx_;
    std::vector<Core> cores_;
    std::deque<std::shared_ptr<Process>> ready_;
    std::multimap<uint64_t, std::shared_ptr<Process>> sleeping_;      // wakeTick -> process
    std::vector<std::shared_ptr<Process>> finished_;                  // in order of completion
    std::unordered_map<std::string, std::shared_ptr<Process>> byName_;

    uint64_t cpuCycles_ = 0;
    uint32_t nextPid_ = 1;
    uint64_t nextAutoIndex_ = 1;
    bool generating_ = false;

private:
    void loop();
    void tick();                       // caller holds mtx_

    void spawnAutoProcess();           // scheduler-start's pN generator
    void wakeSleepers();
    void dispatchIdleCores();

    std::shared_ptr<Process> makeProcess(const std::string& name);
    
    std::atomic<bool> running_{false};
    std::thread thread_;
};
