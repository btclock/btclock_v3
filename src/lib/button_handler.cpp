#include "button_handler.hpp"

TaskHandle_t buttonTaskHandle = NULL;
const TickType_t debounceDelay = pdMS_TO_TICKS(50);
TickType_t lastDebounceTime = 0;

void buttonTask(void *parameter)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        std::lock_guard<std::mutex> lock(mcpMutex);

        TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - lastDebounceTime) >= debounceDelay)
        {
            lastDebounceTime = currentTime;

            if (!digitalRead(MCP_INT_PIN))
            {
                uint pin = mcp1.getLastInterruptPin();

                switch (pin)
                {
                case 3:
                    toggleTimerActive();
                    break;
                case 2:
                    nextScreen();
                    break;
                case 1:
                    previousScreen();
                    break;
                case 0:
                    showSystemStatusScreen();
                    break;
                }
            }
            mcp1.clearInterrupts();
        }
        else
        {
        }
        // Very ugly, but for some reason this is necessary
        while (!digitalRead(MCP_INT_PIN))
        {
            mcp1.clearInterrupts();
        }
    }
}

void IRAM_ATTR handleButtonInterrupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(buttonTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

void setupButtonTask()
{
    xTaskCreate(buttonTask, "ButtonTask", 4096, NULL, tskIDLE_PRIORITY, &buttonTaskHandle); // Create the FreeRTOS task
    // Use interrupt instead of task
    attachInterrupt(MCP_INT_PIN, handleButtonInterrupt, CHANGE);
}
