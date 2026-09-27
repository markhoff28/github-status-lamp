#pragma once
#include <Adafruit_NeoPixel.h>
#include "status_logic.h"
class LedController {
public:
    void begin(uint16_t count, uint8_t brightness = Config::Brightness);
    void setCount(uint16_t count);
    void setBrightness(uint8_t brightness) { brightness_ = brightness; }
    void update(GitHubStatus status, uint32_t now);
private:
    void render(uint8_t red, uint8_t green, uint8_t blue);
    Adafruit_NeoPixel leds_{Config::MaxLedCount, Config::LedPin, NEO_GRB + NEO_KHZ800};
    uint16_t count_ = Config::DefaultLedCount;
    uint8_t brightness_ = Config::Brightness;
    uint32_t lastFrame_ = 0;
};
