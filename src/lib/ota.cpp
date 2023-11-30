#include "ota.hpp"

TaskHandle_t taskOtaHandle = NULL;

void setupOTA() {
  if (preferences.getBool("otaEnabled", true)) {
    ArduinoOTA.onStart(onOTAStart);

    ArduinoOTA.onProgress(onOTAProgress);
    ArduinoOTA.onError(onOTAError);
    ArduinoOTA.onEnd(onOTAComplete);

    ArduinoOTA.setHostname(getMyHostname().c_str());
    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.setRebootOnSuccess(false);
    ArduinoOTA.begin();
    // downloadUpdate();

    xTaskCreate(handleOTATask, "handleOTA", 4096, NULL, tskIDLE_PRIORITY,
                &taskOtaHandle);
  }
}

void onOTAProgress(unsigned int progress, unsigned int total) {
  uint percentage = progress / (total / 100);
  pixels.fill(pixels.Color(0, 255, 0));
  if (percentage < 100) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  }
  if (percentage < 75) {
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
  }
  if (percentage < 50) {
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
  }
  if (percentage < 25) {
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void onOTAStart() {
  forceFullRefresh();
  std::array<String, NUM_SCREENS> epdContent = {"U", "P", "D", "A",
                                                "T", "E", "!"};
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

void handleOTATask(void *parameter) {
  for (;;) {
    ArduinoOTA.handle();  // Allow OTA updates to occur
    vTaskDelay(pdMS_TO_TICKS(2500));
  }
}

void downloadUpdate() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setUserAgent(USER_AGENT);

  // Send HTTP request to CoinGecko API
  http.useHTTP10(true);

  http.begin(client,
             "https://api.github.com/repos/btclock/btclock_v3/releases/latest");
  int httpCode = http.GET();

  if (httpCode == 200) {
    //    WiFiClient * stream = http->getStreamPtr();

    StaticJsonDocument<64> filter;

    JsonObject filter_assets_0 = filter["assets"].createNestedObject();
    filter_assets_0["name"] = true;
    filter_assets_0["browser_download_url"] = true;

    SpiRamJsonDocument doc(1536);

    DeserializationError error = deserializeJson(
        doc, http.getStream(), DeserializationOption::Filter(filter));

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    String downloadUrl;
    for (JsonObject asset : doc["assets"].as<JsonArray>()) {
      if (asset["name"].as<String>().compareTo("firmware.bin") == 0) {
        downloadUrl = asset["browser_download_url"].as<String>();
        break;
      }
    }

    Serial.printf("Download update from %s", downloadUrl);

    // esp_http_client_config_t config = {
    //     .url = CONFIG_FIRMWARE_UPGRADE_URL,
    // };
    // esp_https_ota_config_t ota_config = {
    //     .http_config = &config,
    // };
    // esp_err_t ret = esp_https_ota(&ota_config);
    // if (ret == ESP_OK)
    // {
    //   esp_restart();
    // }
  }
}

void onOTAError(ota_error_t error) {
  Serial.println("\nOTA update error, restarting");
  Wire.end();
  SPI.end();
  delay(1000);
  ESP.restart();
}

void onOTAComplete() {
  Serial.println("\nOTA update finished");
  Wire.end();
  SPI.end();
  delay(1000);
  ESP.restart();
}
