#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_timer.h>
#include <esp_websocket_client.h>

#include <cstring>
#include <string>

#include "lib/led_handler.hpp"
#include "lib/screen_handler.hpp"
#include "lib/timers.hpp"
#include "lib/shared.hpp"

// using namespace websockets;

void setupBlockNotify();

void onWebsocketBlockEvent(void *handler_args, esp_event_base_t base,
                      int32_t event_id, void *event_data);
void onWebsocketBlockMessage(esp_websocket_event_data_t *event_data);

void setBlockHeight(uint newBlockHeight);
uint getBlockHeight();

void setBlockMedianFee(uint blockMedianFee);
uint getBlockMedianFee();

bool isBlockNotifyConnected();
void stopBlockNotify();
void restartBlockNotify();

void processNewBlock(uint newBlockHeight);
void processNewBlockFee(uint newBlockFee);

bool getBlockNotifyInit();
uint getLastBlockUpdate();
int getBlockFetch();
void setLastBlockUpdate(uint lastUpdate);