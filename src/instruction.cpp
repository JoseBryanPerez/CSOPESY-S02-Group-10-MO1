#include "instruction.h"

Symbols::Symbols() {
    texts_.push_back("");  // id 0 = default message placeholder
}

Symbols& Symbols::instance() {
    static Symbols s;
    return s;
}

uint16_t Symbols::internVar(const std::string& name) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = varIds_.find(name);
    if (it != varIds_.end()) return it->second;
    uint16_t id = static_cast<uint16_t>(varNames_.size());
    varNames_.push_back(name);
    varIds_.emplace(name, id);
    return id;
}

std::string Symbols::varName(uint16_t id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return id < varNames_.size() ? varNames_[id] : std::string("?");
}

uint32_t Symbols::internText(const std::string& text) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = textIds_.find(text);
    if (it != textIds_.end()) return it->second;
    uint32_t id = static_cast<uint32_t>(texts_.size());
    texts_.push_back(text);
    textIds_.emplace(text, id);
    return id;
}

std::string Symbols::text(uint32_t id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return id < texts_.size() ? texts_[id] : std::string();
}
