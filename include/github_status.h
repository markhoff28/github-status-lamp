#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "status_logic.h"
struct StatusResult { GitHubStatus status; uint32_t checkedAt; };
class GitHubStatusClient {
public:
    bool begin();
    bool takeResult(StatusResult& result);
private:
    static void run(void* context);
    GitHubStatus fetch();
    QueueHandle_t results_ = nullptr;
};
