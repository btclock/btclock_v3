#pragma once

#include <cstring>
#include <string>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "shared.hpp"

#include "esp_websocket_client.h"
#include "screen_handler.hpp"
#include "led_handler.hpp"

//using namespace websockets;

void setupBlockNotify();

void onWebsocketEvent(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void onWebsocketMessage(esp_websocket_event_data_t* event_data);

unsigned long getBlockHeight();
bool isBlockNotifyConnected();
