#pragma once
#include <stdint.h>
#include <string.h>
#include <ArduinoJson.h>
#include "config.h"

enum class GitHubStatus { Operational, Minor, Major, Critical, Unknown };
inline const char* statusName(GitHubStatus status) {
    switch (status) {
        case GitHubStatus::Operational: return "Operational";
        case GitHubStatus::Minor: return "Minor";
        case GitHubStatus::Major: return "Major";
        case GitHubStatus::Critical: return "Critical";
        default: return "Unknown";
    }
}
inline GitHubStatus parseStatus(const char* json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return GitHubStatus::Unknown;
    const char* indicator = doc["status"]["indicator"] | "";
    if (strcmp(indicator, "none") == 0) return GitHubStatus::Operational;
    if (strcmp(indicator, "minor") == 0) return GitHubStatus::Minor;
    if (strcmp(indicator, "major") == 0) return GitHubStatus::Major;
    if (strcmp(indicator, "critical") == 0) return GitHubStatus::Critical;
    return GitHubStatus::Unknown;
}
inline bool parseLedCount(const char* text, uint16_t& result) {
    if (!text || !*text) return false;
    uint32_t count = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        count = count * 10 + (*p - '0');
        if (count > Config::MaxLedCount) return false;
    }
    if (count == 0) return false;
    result = static_cast<uint16_t>(count);
    return true;
}
inline bool statusIsFresh(bool received, uint32_t lastResult, uint32_t now) {
    return received && static_cast<uint32_t>(now - lastResult) < Config::StaleAfterMs;
}
inline bool parseBrightness(const char* text, uint8_t& result) {
    if (!text || !*text) return false;
    uint32_t value = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        value = value * 10 + (*p - '0');
        if (value > 255) return false;
    }
    result = static_cast<uint8_t>(value);
    return true;
}
