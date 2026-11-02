#include "scheduler.h"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>

#include "util.h"

Scheduler::Scheduler(const Config& cfg, uint64_t tickIntervalUs)
    : cfg_(cfg), tickIntervalUs_(tickIntervalUs), generator_(cfg) {
    const uint64_t n = std::max<uint64_t>(cfg_.numCpu, 1);   // defensive: config values are not range-checked
    cores_.resize(n);
    for (uint64_t i = 0; i < n; ++i) cores_[i].id = static_cast<int>(i);
}

Scheduler::~Scheduler() { stop(); }

void Scheduler::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread(&Scheduler::loop, this);
}

void Scheduler::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

// ---------------------------------------------------------------------------------------------
// Tick loop
// ---------------------------------------------------------------------------------------------

void Scheduler::loop() {
    using clock = std::chrono::steady_clock;
    const auto interval = std::chrono::microseconds(tickIntervalUs_);
    auto next = clock::now();
    while (running_) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            tick();
        }
        if (tickIntervalUs_ == 0) {          // free-running: as fast as possible
            std::this_thread::yield();
            continue;
        }
        // Deadline-based sleep so timer granularity (esp. on Windows) doesn't slow the
        // average tick rate; if we fall more than 100ms behind, stop trying to catch up.
        next += interval;
        const auto now = clock::now();
        if (next + std::chrono::milliseconds(100) < now) next = now;
        std::this_thread::sleep_until(next);
    }
}

void Scheduler::tick() {
    ++cpuCycles_;
    if (generating_ && cfg_.batchProcessFreq > 0 && cpuCycles_ % cfg_.batchProcessFreq == 0) {
        spawnAutoProcess();
    }
    wakeSleepers();
    dispatchIdleCores();
    runCores();
}

std::shared_ptr<Process> Scheduler::makeProcess(const std::string& name) {
    auto p = std::make_shared<Process>(nextPid_++, name, generator_.pickLength());
    byName_[name] = p;
    ready_.push_back(p);
    return p;
}

void Scheduler::spawnAutoProcess() {
    // Human-readable names: p01, p02, ... p1240 (skips any name the user already took via screen -s)
    std::string name;
    do {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "p%02llu", static_cast<unsigned long long>(nextAutoIndex_++));
        name = buf;
    } while (byName_.count(name));
    makeProcess(name);
}

void Scheduler::wakeSleepers() {
    while (!sleeping_.empty() && sleeping_.begin()->first <= cpuCycles_) {
        std::shared_ptr<Process> p = sleeping_.begin()->second;
        sleeping_.erase(sleeping_.begin());
        p->state = ProcessState::Ready;
        ready_.push_back(p);
    }
}

void Scheduler::dispatchIdleCores() {
    for (Core& core : cores_) {
        if (core.busy() || ready_.empty()) continue;

        std::shared_ptr<Process> p = ready_.front();
        ready_.pop_front();

        core.proc = p;
        p->state = ProcessState::Running;
        p->coreId = core.id;
        if (p->startTime == 0) p->startTime = std::time(nullptr);
        if (!p->hasProgram()) p->setProgram(generator_.generate(p->totalInstructions()));
    }
}

void Scheduler::runCores() {
    for (Core& core : cores_) {
        if (!core.busy()) continue;
        Process& p = *core.proc;

        // e.g. a process whose final instruction was SLEEP: nothing left to run.
        if (p.isComplete()) {
            finishProcess(core);
            continue;
        }

        // delays-per-exec: busy-waiting -- the process keeps the core but executes nothing.
        if (p.delayRemaining > 0) {
            --p.delayRemaining;
            continue;
        }

        StepResult r = p.executeNext(core.id);
        p.delayRemaining = cfg_.delaysPerExec;
        if (r == StepResult::Finished) {
            finishProcess(core);
        } else if (r == StepResult::Sleep) {
            sleepProcess(core, p.sleepTicks());
        }
    }
}

void Scheduler::finishProcess(Core& core) {
    std::shared_ptr<Process> p = core.proc;
    p->state = ProcessState::Finished;
    p->finishTime = std::time(nullptr);
    p->coreId = -1;
    p->releaseProgram();
    finished_.push_back(p);
    core.proc.reset();
}

void Scheduler::sleepProcess(Core& core, uint8_t ticks) {
    std::shared_ptr<Process> p = core.proc;
    p->state = ProcessState::Sleeping;
    p->coreId = -1;
    p->delayRemaining = 0;   // sleeping already is a wait; don't also burn a busy-wait after waking
    p->wakeTick = cpuCycles_ + ticks;
    sleeping_.emplace(p->wakeTick, p);
    core.proc.reset();
}

// ---------------------------------------------------------------------------------------------
// Public API used by the CLI thread
// ---------------------------------------------------------------------------------------------

void Scheduler::setGenerating(bool on) {
    std::lock_guard<std::mutex> lk(mtx_);
    generating_ = on;
}

bool Scheduler::generating() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return generating_;
}

uint64_t Scheduler::cpuCycles() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return cpuCycles_;
}

std::string Scheduler::buildUtilizationReport() const {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* rule = "--------------------------------------";

    size_t used = 0;
    for (const Core& c : cores_) used += c.busy() ? 1 : 0;
    const size_t total = cores_.size();
    const size_t pct = total ? used * 100 / total : 0;

    std::ostringstream out;
    out << "CPU utilization: " << pct << "%\n";
    out << "Cores used: " << used << "\n";
    out << "Cores available: " << (total - used) << "\n\n";
    out << rule << "\n";

    out << "Running processes:\n";
    for (const Core& c : cores_) {
        if (!c.busy()) continue;
        const Process& p = *c.proc;
        out << std::left << std::setw(12) << p.name() << " (" << util::formatTime(p.startTime) << ")    Core: "
            << std::setw(6) << c.id << p.instructionsExecuted() << " / " << p.totalInstructions() << "\n";
    }

    out << "\nFinished processes:\n";
    for (const auto& p : finished_) {
        out << std::left << std::setw(12) << p->name() << " (" << util::formatTime(p->startTime)
            << ")    Finished     " << p->instructionsExecuted() << " / " << p->totalInstructions() << "\n";
    }
    out << rule << "\n";
    return out.str();
}
