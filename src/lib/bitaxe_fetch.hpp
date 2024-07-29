#pragma once

#include <Arduino.h>
#include <HTTPClient.h>

#include "lib/config.hpp"
#include "lib/shared.hpp"

extern TaskHandle_t bitaxeFetchTaskHandle;

void setupBitaxeFetchTask();
void taskBitaxeFetch(void *pvParameters);

std::string getBitAxeHashRate();
std::string getBitaxeBestDiff();