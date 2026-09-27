#include "led_controller.h"
void LedController::begin(uint16_t count, uint8_t brightness) {
    leds_.begin();
    setCount(count);
    setBrightness(brightness);
    render(0, 0, 255);
}
void LedController::setCount(uint16_t count) {
    if (count < 1 || count > Config::MaxLedCount) return;
    count_ = count;
    // Clear the unused tail, including LEDs from a previous longer setting.
    leds_.clear();
}
void LedController::render(uint8_t red, uint8_t green, uint8_t blue) {
    // Conservative estimate: 1 mA idle per configured LED, 20 mA per full channel.
    // Rebuild every pixel after setBrightness; NeoPixel rescales its stored buffer.
    const uint32_t channels = static_cast<uint32_t>(red) + green + blue;
    const uint32_t availableMa = Config::PowerLimitMa > count_ ? Config::PowerLimitMa - count_ : 0;
    uint32_t brightness = brightness_;
    if (channels != 0) {
        const uint32_t limit = availableMa * 255UL * 255UL / (count_ * channels * 20UL);
        if (brightness > limit) brightness = limit;
    }
    leds_.setBrightness(static_cast<uint8_t>(brightness));
    leds_.clear();
    leds_.fill(leds_.Color(red, green, blue), 0, count_);
    leds_.show();
}
void LedController::update(GitHubStatus status, uint32_t now) {
    if (static_cast<uint32_t>(now - lastFrame_) < 20) return;
    lastFrame_ = now;
    uint8_t red = 0, green = 0, blue = 255;
    uint32_t period = 0;
    switch (status) {
        case GitHubStatus::Operational: green = 255; blue = 0; break;
        case GitHubStatus::Minor: red = green = 255; blue = 0; period = 3000; break;
        case GitHubStatus::Major: red = 255; green = 70; blue = 0; period = 1500; break;
        case GitHubStatus::Critical:
            red = (now % 800 < 400) ? 255 : 32; blue = 0; break;
        default: break;
    }
    if (period) {
        uint32_t phase = now % period;
        uint32_t triangle = phase < period / 2 ? phase : period - phase;
        const uint32_t scale = 48 + triangle * 207 / (period / 2);
        red = red * scale / 255;
        green = green * scale / 255;
        blue = blue * scale / 255;
    }
    render(red, green, blue);
}
