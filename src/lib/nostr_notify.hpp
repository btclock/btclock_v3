#pragma once

#include "shared.hpp"

#include <ArduinoJson.h>
#include <esp32/ESP32Platform.h>
#include <NostrEvent.h>
#include <NostrPool.h>
#include <Transport.h>
#include <Utils.h>
#include "price_notify.hpp"
#include "block_notify.hpp"

void setupNostrNotify();

void nostrTask(void *pvParameters);
void setupNostrTask();

boolean nostrConnected();