#include "button_handler.hpp"

TaskHandle_t buttonTaskHandle = NULL;
const TickType_t debounceDelay = pdMS_TO_TICKS(50);
TickType_t lastDebounceTime = 0;

#ifdef IS_BTCLOCK_V8
#define BTN_1 0
#define BTN_2 1
#define BTN_3 2
#define BTN_4 3
#else
#define BTN_1 3
#define BTN_2 2
#define BTN_3 1
#define BTN_4 0
#endif

void buttonTask(void *parameter) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    std::lock_guard<std::mutex> lock(mcpMutex);

    TickType_t currentTime = xTaskGetTickCount();
    if ((currentTime - lastDebounceTime) >= debounceDelay) {
      lastDebounceTime = currentTime;

      if (!digitalRead(MCP_INT_PIN)) {
        uint pin = mcp1.getLastInterruptPin();

        switch (pin) {
          case BTN_1:
            toggleTimerActive();
            break;
          case BTN_2:
            nextScreen();
            break;
          case BTN_3:
            previousScreen();
            break;
          case BTN_4:
            showSystemStatusScreen();
            break;
        }
      }
      mcp1.clearInterrupts();
    } else {
    }
    // Very ugly, but for some reason this is necessary
    while (!digitalRead(MCP_INT_PIN)) {
      mcp1.clearInterrupts();
    }
  }
}

void IRAM_ATTR handleButtonInterrupt() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTaskNotifyFromISR(buttonTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

void setupButtonTask() {
  xTaskCreate(buttonTask, "ButtonTask", 3072, NULL, tskIDLE_PRIORITY,
              &buttonTaskHandle);  // Create the FreeRTOS task
  // Use interrupt instead of task
  attachInterrupt(MCP_INT_PIN, handleButtonInterrupt, CHANGE);
}
