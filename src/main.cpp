#include <Arduino.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include "config.h"
#include "github_status.h"
#include "led_controller.h"

namespace {
Preferences preferences;
WiFiManager wifiManager;
WiFiManagerParameter ledParameter("led_count", "Anzahl LEDs (1-300)", "30", 10,
    "type=\"number\" min=\"1\" max=\"300\" required");
WiFiManagerParameter brightnessParameter("brightness", "Helligkeit (0-255, 0 = aus)", "50", 10,
    "type=\"number\" min=\"0\" max=\"255\" required");
LedController leds;
GitHubStatusClient github;
GitHubStatus currentStatus = GitHubStatus::Unknown;
GitHubStatus displayedStatus = GitHubStatus::Unknown;
uint32_t lastResultAt = 0;
bool receivedResult = false;
bool storageReady = false;
bool manualTest = false;
GitHubStatus testStatus = GitHubStatus::Unknown;
uint32_t testStarted = 0;
uint32_t lastReconnect = 0;
uint32_t disconnectedAt = 0;
bool wasConnected = false;
bool mdnsStarted = false;

void startPortal() {
    if (wifiManager.getConfigPortalActive()) return;
    wifiManager.startConfigPortal(Config::SetupSsid, Config::SetupPassword);
    if (wifiManager.getConfigPortalActive() && (WiFi.getMode() & WIFI_AP)) {
        Serial.printf("[Setup] AP active: %s | IP: %s\n",
            Config::SetupSsid, WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[Setup] ERROR: Access point did not start");
    }
}
void saveParameters() {
    uint16_t count;
    uint8_t brightness;
    if (!parseLedCount(ledParameter.getValue(), count) ||
        !parseBrightness(brightnessParameter.getValue(), brightness)) {
        Serial.println("[Setup] Invalid settings; previous values retained (LEDs 1-300, brightness 0-255)");
        return;
    }
    // Store both values atomically in one NVS entry.
    const uint32_t settings = count | (static_cast<uint32_t>(brightness) << 16);
    if (!storageReady || preferences.putUInt("led_settings", settings) != sizeof(uint32_t)) {
        Serial.println("[Setup] Failed to save LED settings");
        return;
    }
    leds.setCount(count);
    leds.setBrightness(brightness);
    Serial.printf("[Setup] Saved: %u LEDs, brightness %u/255\n", count, brightness);
}
void handleControls(uint32_t now) {
    static bool buttonDown = false;
    static bool handled = false;
    static uint32_t pressedAt = 0;
    if (digitalRead(Config::SetupButton) == LOW) {
        if (!buttonDown) { buttonDown = true; pressedAt = now; }
        if (!handled && static_cast<uint32_t>(now - pressedAt) >= 3000) {
            handled = true;
            startPortal();
        }
    } else { buttonDown = false; handled = false; }
    if (Serial.available()) {
        char command = Serial.read();
        if (command == 'p') startPortal();
        else if (command == 's') {
            Serial.printf("[Setup] portal=%d mode=%d AP=%s clients=%u BOOT=%d uptime=%lu\n",
                wifiManager.getConfigPortalActive(), static_cast<int>(WiFi.getMode()),
                WiFi.softAPIP().toString().c_str(), WiFi.softAPgetStationNum(),
                digitalRead(Config::SetupButton), static_cast<unsigned long>(now));
        }
        else if (command == 'a') { manualTest = false; Serial.println("[Test] Live status restored"); }
        else if (command >= '0' && command <= '4') {
            const GitHubStatus states[] = {GitHubStatus::Unknown, GitHubStatus::Operational,
                GitHubStatus::Minor, GitHubStatus::Major, GitHubStatus::Critical};
            testStatus = states[command - '0'];
            manualTest = true;
            testStarted = now;
            Serial.printf("[Test] %s for 30 seconds (a = live)\n", statusName(testStatus));
        }
    }
    if (manualTest && static_cast<uint32_t>(now - testStarted) >= 30000) {
        manualTest = false;
        Serial.println("[Test] Live status restored");
    }
}
}
void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    pinMode(Config::SetupButton, INPUT_PULLUP);
    storageReady = preferences.begin("octolamp", false);
    uint16_t count = storageReady ? preferences.getUShort("led_count", Config::DefaultLedCount) : Config::DefaultLedCount;
    uint8_t brightness = Config::Brightness;
    if (storageReady && preferences.isKey("led_settings")) {
        const uint32_t settings = preferences.getUInt("led_settings");
        count = settings & 0xffff;
        brightness = (settings >> 16) & 0xff;
    }
    if (count == 0 || count > Config::MaxLedCount) count = Config::DefaultLedCount;
    char countText[4];
    snprintf(countText, sizeof(countText), "%u", count);
    ledParameter.setValue(countText, 10);
    char brightnessText[4];
    snprintf(brightnessText, sizeof(brightnessText), "%u", brightness);
    brightnessParameter.setValue(brightnessText, 10);
    leds.begin(count, brightness);
    Serial.printf("[LED] Loaded: %u LEDs, brightness %u/255\n", count, brightness);
    Serial.printf("\nGitHub Status Lamp | XIAO ESP32C3 | NeoPixel | D2/GPIO4 | %u LEDs\n", count);
    Serial.println("Commands: p=setup, s=diagnostics, 0=blue, 1=green, 2=yellow, 3=orange, 4=red, a=live");
    wifiManager.setDebugOutput(false);
    wifiManager.setHostname(Config::Hostname);
    wifiManager.setConfigPortalBlocking(false);
    wifiManager.setConnectTimeout(15);
    wifiManager.setSaveConnectTimeout(15);
    wifiManager.setConfigPortalTimeout(0);
    wifiManager.setDisableConfigPortal(false);
    wifiManager.setBreakAfterConfig(false);
    wifiManager.setSaveParamsCallback(saveParameters);
    wifiManager.addParameter(&ledParameter);
    wifiManager.addParameter(&brightnessParameter);
    std::vector<const char*> menu = {"wifi", "param", "close"};
    wifiManager.setMenu(menu);
    wifiManager.autoConnect(Config::SetupSsid, Config::SetupPassword);
    WiFi.setAutoReconnect(true);
    // Keep setup reachable through both AP and LAN after every reboot.
    startPortal();
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
    if (!github.begin()) Serial.println("[API] Worker could not start; lamp stays Unknown");
    disconnectedAt = millis();
}
void loop() {
    wifiManager.process();
    uint32_t now = millis();
    const bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !wasConnected) {
        Serial.printf("[WiFi] Connected, IP: %s; waiting for time sync / HTTPS status\n", WiFi.localIP().toString().c_str());
        if (!mdnsStarted) {
            mdnsStarted = MDNS.begin(Config::Hostname);
            if (mdnsStarted) {
                MDNS.addService("http", "tcp", 80);
                Serial.printf("[WiFi] Setup: http://%s.local\n", Config::Hostname);
            } else Serial.println("[WiFi] mDNS hostname failed");
        }
    }
    if (!connected && wasConnected) {
        disconnectedAt = now;
        if (mdnsStarted) { MDNS.end(); mdnsStarted = false; }
        Serial.println("[WiFi] Connection lost");
    }
    if (!connected) {
        if (static_cast<uint32_t>(now - disconnectedAt) >= Config::ReconnectIntervalMs) startPortal();
        if (static_cast<uint32_t>(now - lastReconnect) >= Config::ReconnectIntervalMs) {
            lastReconnect = now;
            WiFi.reconnect();
        }
    }
    wasConnected = connected;
    StatusResult result;
    if (github.takeResult(result)) {
        currentStatus = result.status;
        lastResultAt = result.checkedAt;
        receivedResult = true;
    }
    now = millis(); // A queued result may be newer than the start of this loop.
    if (!statusIsFresh(receivedResult, lastResultAt, now)) {
        currentStatus = GitHubStatus::Unknown;
        receivedResult = false;
    }
    GitHubStatus liveStatus = currentStatus;
    handleControls(now);
    GitHubStatus shown = manualTest ? testStatus : liveStatus;
    if (shown != displayedStatus) {
        Serial.printf("[LED] %s -> %s%s\n", statusName(displayedStatus), statusName(shown), manualTest ? " (TEST)" : "");
        displayedStatus = shown;
    }
    leds.update(shown, now);
    vTaskDelay(1); // Yield one RTOS tick to networking and the idle task.
}
