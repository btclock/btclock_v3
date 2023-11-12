#include <Arduino.h>
#include <HTTPClient.h>
#include "config.hpp"
#include "shared.hpp"

extern TaskHandle_t priceFetchTaskHandle;

void setupPriceFetchTask();
void taskPriceFetch(void *pvParameters);