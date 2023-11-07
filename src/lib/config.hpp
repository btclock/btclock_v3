#pragma once;
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <Adafruit_MCP23X17.h>

#include "shared.hpp"
#include <esp_system.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include "epd.hpp"

#include "lib/screen_handler.hpp"
#include "lib/webserver.hpp"
#include "lib/block_notify.hpp"
#include "lib/price_notify.hpp"


void setup();
void setupTime();
void setupPreferences();
void setupWifi();
void setupWebsocketClients();
void setupHardware();