#pragma once
// The command-line interpreter (main menu console).
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "scheduler.h"
#include "round_robin_scheduler.h"

class Console {
public:
    Console(std::string configPath, uint64_t tickIntervalUs);
    void run();   // blocks until "exit" (or EOF on stdin)

private:
    // main menu
    void printBanner() const;
    bool dispatch(const std::vector<std::string>& args);   // returns false when the console should quit
    void cmdInitialize();
    void cmdScreen(const std::vector<std::string>& args);
    void cmdSchedulerStart();
    void cmdSchedulerStop();
    void cmdReportUtil();

    // screen sub-commands
    void screenList();
    void showProcessInformation(const std::string& processName);

    // helpers
    static void clearScreen();
    static bool readLine(const std::string& prompt, std::string& line);   // false on EOF

    std::string configPath_;
    uint64_t tickIntervalUs_;
    std::unique_ptr<Scheduler> scheduler_;
    bool initialized_ = false;
};
