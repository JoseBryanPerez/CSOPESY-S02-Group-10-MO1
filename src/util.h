#pragma once
// Small helpers shared across the emulator.
#include <ctime>
#include <string>
#include <vector>

namespace util {

// "01/18/2024 09:15:22AM" -- the timestamp format used in the spec mockups.
std::string formatTime(std::time_t t);

std::string trim(const std::string& s);
std::string toLower(std::string s);
std::vector<std::string> splitWhitespace(const std::string& s);

// Removes one pair of surrounding double quotes, if present ("rr" -> rr).
std::string stripQuotes(const std::string& s);

}  // namespace util
