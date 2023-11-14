#pragma once
#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include "AsyncJson.h"

#include "lib/block_notify.hpp"
#include "lib/price_notify.hpp"
#include "lib/screen_handler.hpp"
#include "lib/led_handler.hpp"

#include "webserver/OneParamRewrite.hpp"

extern TaskHandle_t eventSourceTaskHandle;

void stopWebServer();
void setupWebserver();
bool processEpdColorSettings(AsyncWebServerRequest *request);

void onApiStatus(AsyncWebServerRequest *request);
void onApiSystemStatus(AsyncWebServerRequest *request);

void onApiShowScreen(AsyncWebServerRequest *request);
void onApiShowText(AsyncWebServerRequest *request);
void onApiShowTextAdvanced(AsyncWebServerRequest *request, JsonVariant &json);

void onApiActionPause(AsyncWebServerRequest *request);
void onApiActionTimerRestart(AsyncWebServerRequest *request);
void onApiSettingsGet(AsyncWebServerRequest *request);
void onApiSettingsPost(AsyncWebServerRequest *request);
void onApiFullRefresh(AsyncWebServerRequest *request);

void onApiLightsOff(AsyncWebServerRequest *request);
void onApiLightsSetColor(AsyncWebServerRequest *request);


void onApiRestart(AsyncWebServerRequest *request);

void onIndex(AsyncWebServerRequest *request);
void onNotFound(AsyncWebServerRequest *request);

StaticJsonDocument<768> getStatusObject();
void eventSourceUpdate();
void eventSourceTask(void *pvParameters);