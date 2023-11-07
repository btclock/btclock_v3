#pragma once

#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>

#include "lib/block_notify.hpp"
#include "lib/price_notify.hpp"

void setupWebserver();
void onNotFound(AsyncWebServerRequest *request);