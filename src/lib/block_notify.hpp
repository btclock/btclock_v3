#pragma once

#include "lib/led_handler.hpp"
#include "lib/screen_handler.hpp"
#include "lib/shared.hpp"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <cstring>
#include <esp_timer.h>
#include <esp_websocket_client.h>
#include <string>

// using namespace websockets;

void setupBlockNotify();

void onWebsocketEvent(void *handler_args, esp_event_base_t base,
                      int32_t event_id, void *event_data);
void onWebsocketMessage(esp_websocket_event_data_t *event_data);

void setBlockHeight(uint newBlockHeight);
uint getBlockHeight();
bool isBlockNotifyConnected();
void stopBlockNotify();