#pragma once

// Keep order of includes because of conflicts
#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include "AsyncJson.h"
#include <iostream>

#include "lib/block_notify.hpp"
#include "lib/led_handler.hpp"
#include "lib/price_notify.hpp"
#include "lib/screen_handler.hpp"
#include "webserver/OneParamRewrite.hpp"

extern TaskHandle_t eventSourceTaskHandle;

void stopWebServer();
void setupWebserver();
bool processEpdColorSettings(AsyncWebServerRequest *request);

void onApiStatus(AsyncWebServerRequest *request);
void onApiSystemStatus(AsyncWebServerRequest *request);
void onApiSetWifiTxPower(AsyncWebServerRequest *request);

void onApiShowScreen(AsyncWebServerRequest *request);
void onApiShowText(AsyncWebServerRequest *request);
void onApiShowTextAdvanced(AsyncWebServerRequest *request, JsonVariant &json);

void onApiActionPause(AsyncWebServerRequest *request);
void onApiActionTimerRestart(AsyncWebServerRequest *request);
void onApiSettingsGet(AsyncWebServerRequest *request);
void onApiSettingsPost(AsyncWebServerRequest *request);
void onApiSettingsPatch(AsyncWebServerRequest *request, JsonVariant &json);
void onApiFullRefresh(AsyncWebServerRequest *request);

void onApiLightsStatus(AsyncWebServerRequest *request);
void onApiLightsOff(AsyncWebServerRequest *request);
void onApiLightsSetColor(AsyncWebServerRequest *request);
void onApiLightsSetJson(AsyncWebServerRequest *request, JsonVariant &json);

void onApiRestart(AsyncWebServerRequest *request);

void onIndex(AsyncWebServerRequest *request);
void onNotFound(AsyncWebServerRequest *request);

StaticJsonDocument<512> getLedStatusObject();
StaticJsonDocument<768> getStatusObject();
void eventSourceUpdate();
void eventSourceTask(void *pvParameters);

void onApiStopDataSources(AsyncWebServerRequest *request);
void onApiRestartDataSources(AsyncWebServerRequest *request);