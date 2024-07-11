#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_websocket_client.h>
#include "block_notify.hpp"
#include <string>

#include "lib/screen_handler.hpp"

void setupPriceNotify();

void onWebsocketPriceEvent(void *handler_args, esp_event_base_t base,
                           int32_t event_id, void *event_data);
void onWebsocketPriceMessage(esp_websocket_event_data_t *event_data);

uint getPrice();
void setPrice(uint newPrice);

void processNewPrice(uint newPrice);

bool isPriceNotifyConnected();
void stopPriceNotify();
void restartPriceNotify();

bool getPriceNotifyInit();
uint getLastPriceUpdate();