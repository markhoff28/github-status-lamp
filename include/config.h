#pragma once
#include <stdint.h>
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy include/secrets.example.h to include/secrets.h and set your setup Wi-Fi password."
#endif
namespace Config {
constexpr uint8_t LedPin = 4; // XIAO D2
constexpr uint8_t SetupButton = 9; // BOOT button; hold after startup
constexpr uint16_t DefaultLedCount = 30;
constexpr uint16_t MaxLedCount = 300;
constexpr uint8_t Brightness = 50;
constexpr uint16_t PowerLimitMa = 500; // Software estimate, not a fuse
constexpr uint32_t PollIntervalMs = 60000;
constexpr uint32_t StaleAfterMs = 120000;
constexpr uint32_t HttpTimeoutMs = 8000;
constexpr uint32_t ReconnectIntervalMs = 30000;
constexpr char SetupSsid[] = "GitHub-Status-Lamp-Setup";
constexpr char SetupPassword[] = LAMP_SETUP_PASSWORD;
static_assert(sizeof(SetupPassword) >= 9 && sizeof(SetupPassword) <= 64,
    "Setup Wi-Fi password must contain 8-63 characters.");
constexpr char Hostname[] = "github-status-lamp";
constexpr char StatusUrl[] = "https://www.githubstatus.com/api/v2/status.json";
}
