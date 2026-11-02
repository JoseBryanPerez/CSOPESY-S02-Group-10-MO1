// CSOPESY OS Emulator -- Process Scheduler & CLI (MO1)
// Entry point. Usage:  csopesy [--config <file>] [--tick-us <n>]
//   --config   path to the config file read by "initialize"   (default: config.txt)
//   --tick-us  real microseconds per CPU tick                  (default: 1000)
//              Lower = faster emulation (handy for testing); 0 = free-running.
#include <cstdlib>
#include <iostream>
#include <string>

#include "console.h"

int main(int argc, char** argv) {
    std::string configPath = "config.txt";
    uint64_t tickUs = 1000;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--config" && i + 1 < argc) {
            configPath = argv[++i];
        } else if (a == "--tick-us" && i + 1 < argc) {
            tickUs = std::strtoull(argv[++i], nullptr, 10);
        } else {
            std::cerr << "Usage: " << argv[0] << " [--config <file>] [--tick-us <n>]\n";
            return 1;
        }
    }

    Console console(configPath, tickUs);
    console.run();
    return 0;
}
