#include "config.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "util.h"

namespace {

constexpr uint64_t kMaxConfigValue = uint64_t{1} << 32;

bool parseUnsigned(const std::string& text, uint64_t& out) {
    if (text.empty() || text[0] == '-') return false;
    errno = 0;
    char* end = nullptr;
    unsigned long long v = std::strtoull(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0') return false;
    out = v;
    return true;
}

bool validateRange(const std::string& key, uint64_t value, uint64_t min, uint64_t max,
                   int lineNo, std::string& error) {
    if (value >= min && value <= max) return true;
    std::ostringstream message;
    message << "config line " << lineNo << ": '" << key << "' must be in [" << min << ", " << max << "].";
    error = message.str();
    return false;
}

}  // namespace

bool loadConfig(const std::string& path, Config& out, std::string& error) {
    error.clear();
    std::ifstream in(path);
    if (!in) {
        error = "Could not open config file '" + path + "'.";
        return false;
    }

    Config cfg;  // starts at defaults
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = util::trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        key = util::toLower(key);
        value = util::stripQuotes(value);

        if (value.empty()) {
            error = "config line " + std::to_string(lineNo) + ": '" + key + "' has no value.";
            return false;
        }

        std::string extra;
        if (iss >> extra) {
            error = "config line " + std::to_string(lineNo) + ": unexpected extra value '" + extra + "'.";
            return false;
        }

        if (key == "scheduler") {
            cfg.scheduler = util::toLower(value);
            if (cfg.scheduler != "fcfs" && cfg.scheduler != "rr") {
                error = "config line " + std::to_string(lineNo) + ": scheduler must be 'fcfs' or 'rr'.";
                return false;
            }
            continue;
        }

        uint64_t n = 0;
        bool known = true;
        uint64_t* target = nullptr;
        if (key == "num-cpu") target = &cfg.numCpu;
        else if (key == "quantum-cycles") target = &cfg.quantumCycles;
        else if (key == "batch-process-freq") target = &cfg.batchProcessFreq;
        else if (key == "min-ins") target = &cfg.minIns;
        else if (key == "max-ins") target = &cfg.maxIns;
        else if (key == "delays-per-exec" || key == "delay-per-exec") target = &cfg.delaysPerExec;
        else known = false;

        if (!known) {
            std::cerr << "[config] line " << lineNo << ": ignoring unknown key '" << key << "'\n";
            continue;
        }
        if (!parseUnsigned(value, n)) {
            error = "config line " + std::to_string(lineNo) + ": '" + value +
                    "' is not a valid non-negative integer for '" + key + "'.";
            return false;
        }
        uint64_t min = 0;
        uint64_t max = kMaxConfigValue;
        if (key == "num-cpu") {
            min = 1;
            max = 128;
        } else if (key != "delays-per-exec" && key != "delay-per-exec") {
            min = 1;
        }
        if (!validateRange(key, n, min, max, lineNo, error)) return false;
        *target = n;
    }

    if (cfg.minIns > cfg.maxIns) {
        error = "config: 'min-ins' must be less than or equal to 'max-ins'.";
        return false;
    }

    out = cfg;
    return true;
}

std::string describeConfig(const Config& cfg) {
    std::ostringstream o;
    o << "num-cpu=" << cfg.numCpu << " scheduler=" << cfg.scheduler
      << " quantum-cycles=" << cfg.quantumCycles << " batch-process-freq=" << cfg.batchProcessFreq
      << " min-ins=" << cfg.minIns << " max-ins=" << cfg.maxIns
      << " delays-per-exec=" << cfg.delaysPerExec;
    return o.str();
}
