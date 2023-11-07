#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "lib/block_notify.hpp"
#include "lib/price_notify.hpp"
#include "lib/epd.hpp"

extern TaskHandle_t priceUpdateTaskHandle;
extern TaskHandle_t blockUpdateTaskHandle;

void taskPriceUpdate(void *pvParameters);
void taskBlockUpdate(void *pvParameters);

void setupTasks();