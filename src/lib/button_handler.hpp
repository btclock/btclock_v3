#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "shared.hpp"

extern TaskHandle_t buttonTaskHandle;

void buttonTask(void *pvParameters);
void IRAM_ATTR handleButtonInterrupt();
void setupButtonTask();
