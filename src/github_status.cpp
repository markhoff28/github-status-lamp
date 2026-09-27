#include "github_status.h"
#include "github_status_ca.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

namespace {
// HTTPClient 2.x can otherwise wait indefinitely for an idle response stream.
class DeadlineTls : public WiFiClientSecure {
public:
    void startBodyTimer() { started_ = millis(); timed_ = true; }
    bool expired() const {
        return timed_ && static_cast<uint32_t>(millis() - started_) >= Config::HttpTimeoutMs;
    }
    uint8_t connected() override {
        if (expired()) { stop(); return 0; }
        return WiFiClientSecure::connected();
    }
private:
    uint32_t started_ = 0;
    bool timed_ = false;
};
// Let HTTPClient decode both Content-Length and chunked responses, while limiting
// the output buffer independently of untrusted HTTP headers.
class BoundedBody : public Stream {
public:
    size_t write(uint8_t value) override { return write(&value, 1); }
    size_t write(const uint8_t* data, size_t size) override {
        if (size > sizeof(body_) - 1 - used_) { overflow_ = true; return 0; }
        memcpy(body_ + used_, data, size);
        used_ += size;
        body_[used_] = '\0';
        return size;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    const char* text() const { return body_; }
    bool overflow() const { return overflow_; }
private:
    char body_[4097] = {};
    size_t used_ = 0;
    bool overflow_ = false;
};
}
bool GitHubStatusClient::begin() {
    results_ = xQueueCreate(1, sizeof(StatusResult));
    if (!results_) return false;
    if (xTaskCreate(run, "github-status", 16384, this, 1, nullptr) != pdPASS) {
        vQueueDelete(results_); results_ = nullptr; return false;
    }
    return true;
}
bool GitHubStatusClient::takeResult(StatusResult& result) {
    return results_ && xQueueReceive(results_, &result, 0) == pdTRUE;
}
void GitHubStatusClient::run(void* context) {
    auto& client = *static_cast<GitHubStatusClient*>(context);
    bool wasReady = false;
    uint32_t lastPoll = 0;
    for (;;) {
        // Certificate validation requires a clock; SNTP runs asynchronously.
        bool ready = WiFi.status() == WL_CONNECTED && time(nullptr) > 1704067200;
        if (ready && (!wasReady || static_cast<uint32_t>(millis() - lastPoll) >= Config::PollIntervalMs)) {
            GitHubStatus status = client.fetch();
            StatusResult result{status, millis()};
            xQueueOverwrite(client.results_, &result);
            lastPoll = millis();
        }
        wasReady = ready;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
GitHubStatus GitHubStatusClient::fetch() {
    DeadlineTls tls;
    tls.setCACert(kGitHubStatusCa);
    tls.setHandshakeTimeout(8);
    HTTPClient http;
    http.setConnectTimeout(Config::HttpTimeoutMs);
    http.setTimeout(Config::HttpTimeoutMs);
    http.useHTTP10(true);
    if (!http.begin(tls, Config::StatusUrl)) return GitHubStatus::Unknown;
    http.setUserAgent("github-status-lamp/0.1");
    int code = http.GET();
    GitHubStatus status = GitHubStatus::Unknown;
    if (code == HTTP_CODE_OK) {
        BoundedBody body;
        tls.startBodyTimer();
        const int bytes = http.getSize() > 4096 ? -1 : http.writeToStream(&body);
        if (bytes > 0 && !body.overflow() && !tls.expired()) status = parseStatus(body.text());
        else Serial.printf("[API] Response read failed or too large: %d\n", bytes);
    } else Serial.printf("[API] HTTP/transport error: %d\n", code);
    http.end();
    Serial.printf("[API] %s\n", statusName(status));
    return status;
}
