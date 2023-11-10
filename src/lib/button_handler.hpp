#pragma once

#include <Arduino.h>
#include "shared.hpp"
#include "screen_handler.hpp"

extern TaskHandle_t buttonTaskHandle;

void buttonTask(void *pvParameters);
void IRAM_ATTR handleButtonInterrupt();
void setupButtonTask();
