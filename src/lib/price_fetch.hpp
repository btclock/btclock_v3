#include <Arduino.h>
#include <HTTPClient.h>

#include "lib/config.hpp"
#include "lib/shared.hpp"

extern TaskHandle_t priceFetchTaskHandle;

void setupPriceFetchTask();
void taskPriceFetch(void *pvParameters);