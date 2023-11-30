#include "lib/config.hpp"
#include "lib/shared.hpp"
#include <Arduino.h>
#include <HTTPClient.h>

extern TaskHandle_t priceFetchTaskHandle;

void setupPriceFetchTask();
void taskPriceFetch(void *pvParameters);