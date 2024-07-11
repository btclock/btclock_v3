#pragma once

#include <Adafruit_MCP23X17.h>
#include <Arduino.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <base64.h>
#include <esp_task_wdt.h>

#include <map>

#include "lib/block_notify.hpp"
#include "lib/button_handler.hpp"
#include "lib/epd.hpp"
#include "lib/improv.hpp"
#include "lib/led_handler.hpp"
#include "lib/ota.hpp"
#include "lib/price_notify.hpp"
#include "lib/screen_handler.hpp"
#include "lib/shared.hpp"
#include "lib/webserver.hpp"
#ifdef HAS_FRONTLIGHT
#include "PCA9685.h"
#include "BH1750.h"
#endif

#define NTP_SERVER "pool.ntp.org"
#define DEFAULT_TIME_OFFSET_SECONDS 3600
#define USER_AGENT "BTClock/3.0"
#ifndef MCP_DEV_ADDR
#define MCP_DEV_ADDR 0x20
#endif


void setup();
void syncTime();
uint getLastTimeSync();
void setupPreferences();
void setupWebsocketClients(void *pvParameters);
void setupHardware();
void tryImprovSetup();
void setupTimers();
void finishSetup();
void setupMcp();
#ifdef HAS_FRONTLIGHT
void setupFrontlight();
float getLightLevel();
bool hasLightLevel();
extern PCA9685 flArray;
#endif

String getMyHostname();
std::vector<std::string> getScreenNameMap();

std::vector<std::string> getLocalUrl();
bool improv_connectWifi(std::string ssid, std::string password);
void improvGetAvailableWifiNetworks();
bool onImprovCommandCallback(improv::ImprovCommand cmd);
void onImprovErrorCallback(improv::Error err);
void improv_set_state(improv::State state);
void improv_send_response(std::vector<uint8_t> &response);
void improv_set_error(improv::Error error);

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
String getHwRev();
bool isWhiteVersion();
String getFsRev();