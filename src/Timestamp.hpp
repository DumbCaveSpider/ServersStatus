#pragma once

#include <fmt/chrono.h>
#include <Geode/utils/general.hpp>
#include <string>

// Return a local timestamp formatted as "YYYY-MM-DD HH:MM:SS"
static inline std::string getLocalTimestamp() {
    auto now = std::time(nullptr);
    return fmt::format("{:%Y-%m-%d %H:%M:%S}", geode::localtime(now));
}
