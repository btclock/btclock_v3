#include "led_handler.hpp"

TaskHandle_t ledTaskHandle = NULL;
const TickType_t debounceDelay = pdMS_TO_TICKS(50);

void ledTask(void *parameter)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
    }
}

void setupLedTask()
{
    xTaskCreate(ledTask, "LedTask", 4096, NULL, tskIDLE_PRIORITY, &ledTaskHandle); // Create the FreeRTOS task
}
