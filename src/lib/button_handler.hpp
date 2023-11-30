#pragma once

#include <Arduino.h>

#include "lib/screen_handler.hpp"
#include "lib/shared.hpp"

extern TaskHandle_t buttonTaskHandle;

void buttonTask(void *pvParameters);
void IRAM_ATTR handleButtonInterrupt();
void setupButtonTask();
