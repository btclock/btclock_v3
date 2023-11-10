#include "ota.hpp"

TaskHandle_t taskOtaHandle = NULL;


void setupOTA()
{
    ArduinoOTA.onStart(onOTAStart);

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100))); });

    ArduinoOTA.onEnd([]()
                     { Serial.println("\nOTA update finished"); });


    ArduinoOTA.setHostname(getMyHostname().c_str());
    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.begin();

    xTaskCreate(handleOTATask, "handleOTA", 4096, NULL, tskIDLE_PRIORITY, &taskOtaHandle);

}

void onOTAStart()
{
    // Stop all timers
    esp_timer_stop(screenRotateTimer);
    esp_timer_stop(minuteTimer);

    // Stop or suspend all tasks
    vTaskSuspend(priceUpdateTaskHandle);
    vTaskSuspend(blockUpdateTaskHandle);
    vTaskSuspend(timeUpdateTaskHandle);
    vTaskSuspend(taskScreenRotateTaskHandle);

    vTaskSuspend(ledTaskHandle);
    vTaskSuspend(buttonTaskHandle);

    stopWebServer();
    stopBlockNotify();
    stopPriceNotify();
}

void handleOTATask(void *parameter) {
  for (;;) {
    // Task 1 code
    ArduinoOTA.handle();  // Allow OTA updates to occur
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}