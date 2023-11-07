#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Adafruit_NeoPixel.h>
#include "shared.hpp"

extern TaskHandle_t ledTaskHandle;

void ledTask(void *pvParameters);
void setupLedTask();
