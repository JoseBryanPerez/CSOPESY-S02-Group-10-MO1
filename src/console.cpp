#include "console.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "config.h"
#include "util.h"

namespace {

const char* const kPrompt = "root:\\> ";

const char* const kDevelopers = "Adriano, Buenavente, Lee, Perez";
const char* const kLastUpdated = "11-02-2026";

const char* const kLogo = R"LOGO(
   ___ ___  ___  ___ ___ ___ __   __
  / __/ __|/ _ \| _ \ __/ __|\ \ / /
 | (__\__ \ (_) |  _/ _|\__ \ \ V /
  \___|___/\___/|_| |___|___/  |_|
)LOGO";

}  // namespace

Console::Console(std::string configPath, uint64_t tickIntervalUs)
    : configPath_(std::move(configPath)), tickIntervalUs_(tickIntervalUs) {}

void Console::clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H" << std::flush;
#endif
}

bool Console::readLine(const std::string& prompt, std::string& line) {
    std::cout << prompt << std::flush;
    if (!std::getline(std::cin, line)) return false;
    line = util::trim(line);
    return true;
}

void Console::printBanner() const {
    std::cout << kLogo;
    std::cout << "--------------------------------------\n";
    std::cout << "Welcome to CSOPESY Emulator!\n\n";
    std::cout << "Developers:\n" << kDevelopers << "\n\n";
    std::cout << "Last updated: " << kLastUpdated << "\n";
    std::cout << "--------------------------------------\n";
}

void Console::run() {
    printBanner();
    std::string line;
    while (readLine(kPrompt, line)) {
        std::vector<std::string> args = util::splitWhitespace(line);
        if (args.empty()) continue;
        if (!dispatch(args)) break;
    }
    if (scheduler_) scheduler_->stop();   // also covers EOF on stdin
}

bool Console::dispatch(const std::vector<std::string>& args) {
    const std::string cmd = util::toLower(args[0]);

    if (currentScreen_ == Screen::MainMenu) {
        if (cmd == "exit") return false;

        if (cmd == "initialize") {
            cmdInitialize();
            return true;
        }
        
        if (!initialized_) {
            std::cout << "Please run 'initialize' first.\n";
            return true;
        }

        if (cmd == "screen") cmdScreen(args);
        else if (cmd == "scheduler-start" || cmd == "scheduler-test") cmdSchedulerStart();   // old name kept as alias
        else if (cmd == "scheduler-stop") cmdSchedulerStop();
        else if (cmd == "report-util") cmdReportUtil();
        else if (cmd == "clear" || cmd == "cls") clearScreen();
        else std::cout << "Unknown command: " << args[0] << "\n";
    } else if (currentScreen_ == Screen::Process) {
        if (cmd == "process-smi") showProcessInformation(false);
        else if (cmd == "exit") currentScreen_ = Screen::MainMenu;
        else std::cout << "Unknown command: " << args[0] << "\n";
    }
    
    return true;
}

// ---------------------------------------------------------------------------------------------
// initialize
// ---------------------------------------------------------------------------------------------

void Console::cmdInitialize() {
    Config cfg;
    std::string error;
    if (!loadConfig(configPath_, cfg, error)) {
        std::cout << "Initialization failed: " << error << "\n";
        return;
    }

    if (scheduler_) scheduler_->stop();   // re-initialize: throw away the old run
    
    bool showWarning = false;
    if (cfg.scheduler == "rr") {
        scheduler_ = std::make_unique<RoundRobinScheduler>(cfg, tickIntervalUs_);
    } else if (cfg.scheduler == "fcfs") {
        scheduler_ = std::make_unique<Scheduler>(cfg, tickIntervalUs_);
    } else {
        scheduler_ = std::make_unique<Scheduler>(cfg, tickIntervalUs_);
        showWarning = true;
    }
    scheduler_->start();
    initialized_ = true;

    std::cout << "Initialized from " << configPath_ << ": " << describeConfig(cfg) << "\n";
    if (showWarning) {
        std::cout << "[WARN] Scheduler '" << cfg.scheduler
                  << "' is not implemented in this build -- running as FCFS.\n";
    }
}

// ---------------------------------------------------------------------------------------------
// scheduler-start / scheduler-stop / report-util
// ---------------------------------------------------------------------------------------------

void Console::cmdSchedulerStart() {
    if (scheduler_->generating()) {
        std::cout << "Scheduler is already generating processes.\n";
        return;
    }
    scheduler_->setGenerating(true);
    std::cout << "Scheduler started: generating a process every " << scheduler_->config().batchProcessFreq
              << " CPU tick(s).\n";
}

void Console::cmdSchedulerStop() {
    if (!scheduler_->generating()) {
        std::cout << "Scheduler is not generating processes.\n";
        return;
    }
    scheduler_->setGenerating(false);
    std::cout << "Scheduler stopped: no longer generating processes.\n";
}

void Console::cmdReportUtil() {
    const std::string path = "csopesy-log.txt";
    std::ofstream file(path);
    if (!file) {
        std::cout << "Could not write " << path << ".\n";
        return;
    }
    file << scheduler_->buildUtilizationReport();
    file.close();
    std::cout << "Report generated at " << std::filesystem::absolute(path).string() << "!\n";
}

// ---------------------------------------------------------------------------------------------
// screen
// ---------------------------------------------------------------------------------------------

void Console::cmdScreen(const std::vector<std::string>& args) {
    if (args.size() == 2 && args[1] == "-ls") {
        screenList();
    } else if (args.size() >= 2 && args[1] == "-s") {
        currentProcessSMI_ = args[2];
        currentScreen_ = Screen::Process;

        clearScreen();
        showProcessInformation(true);
    } else if (args.size() >= 2 && args[1] == "-r") {
        currentProcessSMI_ = args[2];
        currentScreen_ = Screen::Process;

        showProcessInformation(true);
    } else {
        std::cout << "Usage:\n"
                     "  screen -s <process name>   create a new process and attach to its screen  (not implemented yet)\n"
                     "  screen -r <process name>   re-attach to a running process's screen         (not implemented yet)\n"
                     "  screen -ls                 list CPU utilization and running/finished processes\n";
    }
}

void Console::screenList() { std::cout << scheduler_->buildUtilizationReport(); }

void Console::showProcessInformation(bool excludeFinished) {
    std::cout << scheduler_->buildLogReportFromProcess(currentProcessSMI_, excludeFinished);
}
