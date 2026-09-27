#include <Arduino.h>
#ifdef LED_DIAGNOSTIC_NEOPIXEL
#include <Adafruit_NeoPixel.h>
#else
#include <FastLED.h>
#endif
#include "config.h"

// Standalone hardware check: no Wi-Fi, portal, JSON or background API task.
namespace {
#ifdef LED_DIAGNOSTIC_NEOPIXEL
Adafruit_NeoPixel pixels(Config::MaxLedCount, Config::LedPin, NEO_GRB + NEO_KHZ800);
constexpr const char* DriverName = "Adafruit NeoPixel";
#else
CRGB pixels[Config::MaxLedCount];
constexpr const char* DriverName = "FastLED";
#endif
constexpr uint16_t TestCount = 10;
uint32_t lastStep = 0;
uint8_t step = 0;
const char* const names[] = {"OFF", "RED", "GREEN", "BLUE"};

void displayStep() {
#ifdef LED_DIAGNOSTIC_NEOPIXEL
    pixels.clear();
    const uint32_t colors[] = {0, pixels.Color(255, 0, 0), pixels.Color(0, 255, 0), pixels.Color(0, 0, 255)};
    pixels.fill(colors[step], 0, TestCount);
#else
    fill_solid(pixels, Config::MaxLedCount, CRGB::Black);
    if (step != 0) {
        const CRGB colors[] = {CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Blue};
        fill_solid(pixels, TestCount, colors[step]);
    }
#endif
    Serial.printf("[LED diagnostic / %s] D2/GPIO4, first %u LEDs, brightness 32: %s\n", DriverName, TestCount, names[step]);
}
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
#ifdef LED_DIAGNOSTIC_NEOPIXEL
    pixels.begin();
    pixels.setBrightness(32);
#else
    FastLED.addLeds<WS2812B, Config::LedPin, GRB>(pixels, Config::MaxLedCount);
    FastLED.setBrightness(32);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, Config::PowerLimitMa);
#endif
    displayStep();
    lastStep = millis();
}

void loop() {
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastStep) >= 5000) {
        lastStep = now;
        step = (step + 1) % 4;
        displayStep();
    }
    // Repeat each frame with a generous latch interval between transmissions.
#ifdef LED_DIAGNOSTIC_NEOPIXEL
    pixels.show();
#else
    FastLED.show();
#endif
    delay(20); // Intentional in this standalone diagnostic, not production animation.
}
