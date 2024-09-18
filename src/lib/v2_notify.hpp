#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_websocket_client.h>
#include "block_notify.hpp"
#include <string>

#include "lib/screen_handler.hpp"

namespace V2Notify {
    extern TaskHandle_t v2NotifyTaskHandle;

    void setupV2NotifyTask();
    void taskV2Notify(void *pvParameters);

    void setupV2Notify();
    void onWebsocketV2Event(WStype_t type, uint8_t * payload, size_t length);
    void handleV2Message(JsonDocument doc);

    bool isV2NotifyConnected();
}
// void stopV2Notify();
// void restartV2Notify();
// bool getPriceNotifyInit();
// uint getLastPriceUpdate();