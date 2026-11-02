#include "config.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "util.h"

namespace {

bool parseUnsigned(const std::string& text, uint64_t& out) {
    if (text.empty() || text[0] == '-') return false;
    errno = 0;
    char* end = nullptr;
    unsigned long long v = std::strtoull(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0') return false;
    out = v;
    return true;
}

}  // namespace

bool loadConfig(const std::string& path, Config& out, std::string& error) {
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

        if (key == "scheduler") {
            cfg.scheduler = util::toLower(value);
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
        *target = n;
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
