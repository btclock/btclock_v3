#include "ota.hpp"

TaskHandle_t taskOtaHandle = NULL;

void setupOTA()
{
  if (preferences.getBool("otaEnabled", true))
  {
    ArduinoOTA.onStart(onOTAStart);

    ArduinoOTA.onProgress(onOTAProgress);
    ArduinoOTA.onError(onOTAError);
    ArduinoOTA.onEnd(onOTAComplete);

    ArduinoOTA.setHostname(getMyHostname().c_str());
    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.setRebootOnSuccess(false);
    ArduinoOTA.begin();

    xTaskCreate(handleOTATask, "handleOTA", 4096, NULL, tskIDLE_PRIORITY, &taskOtaHandle);
  }
}

void onOTAProgress(unsigned int progress, unsigned int total)
{
  uint percentage = progress / (total / 100);
  pixels.fill(pixels.Color(0, 255, 0));
  if (percentage < 100)
  {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  }
  if (percentage < 75)
  {
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
  }
  if (percentage < 50)
  {
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
  }
  if (percentage < 25)
  {
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void onOTAStart()
{
  forceFullRefresh();
  std::array<String, NUM_SCREENS> epdContent = {"U", "P", "D", "A", "T", "E", "!"};
  setEpdContent(epdContent);
  // Stop all timers
  esp_timer_stop(screenRotateTimer);
  esp_timer_stop(minuteTimer);

  // Stop or suspend all tasks
  //  vTaskSuspend(priceUpdateTaskHandle);
  //    vTaskSuspend(blockUpdateTaskHandle);
  vTaskSuspend(workerTaskHandle);
  vTaskSuspend(taskScreenRotateTaskHandle);

  vTaskSuspend(ledTaskHandle);
  vTaskSuspend(buttonTaskHandle);

  stopWebServer();
  stopBlockNotify();
  stopPriceNotify();
}

void handleOTATask(void *parameter)
{
  for (;;)
  {
    ArduinoOTA.handle(); // Allow OTA updates to occur
    vTaskDelay(pdMS_TO_TICKS(2500));
  }
}

void downloadUpdate()
{
}

void onOTAError(ota_error_t error) {
  Serial.println("\nOTA update error, restarting");
  Wire.end();
  SPI.end();
  delay(1000);
  ESP.restart(); 
}

void onOTAComplete()
{
  Serial.println("\nOTA update finished");
  Wire.end();
  SPI.end();
  delay(1000);
  ESP.restart();
}
